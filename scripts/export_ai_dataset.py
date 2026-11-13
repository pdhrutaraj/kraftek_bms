#!/usr/bin/env python3

import os
import sys
import csv
import json
import sqlite3
import argparse
import datetime

DEFAULT_DB = (
    "/home/user/.local/share/Kraftek/"
    "Kraftek BMS Console/kraftek_bms.sqlite3"
)

def write_csv(path, query, columns, connection):
    count = 0
    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(columns)
        cursor = connection.execute(query)
        for row in cursor:
            writer.writerow(row)
            count += 1
    return count


def assess_provenance(quality_counts):
    source_counts = {}
    for source, quality, count in quality_counts:
        source_counts[source] = source_counts.get(source, 0) + int(count)

    total = sum(source_counts.values())
    reasons = []

    if total == 0:
        status = "NO_TELEMETRY"
        reasons.append("The telemetry dataset is empty.")
    else:
        has_simulator = "SIMULATOR" in source_counts
        other_sources = [
            source for source in source_counts
            if source != "SIMULATOR"
        ]

        if has_simulator and not other_sources:
            status = "SIMULATOR_ONLY"
        elif has_simulator:
            status = "MIXED_PROVENANCE"
        else:
            status = "UNVERIFIED_SOURCE_PRESENT"

        if has_simulator:
            reasons.append("Simulator records are present.")

        for source in other_sources:
            if source == "RXN_BMS":
                reasons.append(
                    "RXN_BMS is currently a placeholder with no verified "
                    "serial telemetry implementation."
                )
            elif source is None or not str(source).strip():
                reasons.append(
                    "Telemetry contains a missing or blank data source."
                )
            else:
                reasons.append(
                    "Source %r is not approved by the hardware provenance "
                    "policy." % source
                )

    reasons.append(
        "No hardware source is approved by the current provenance policy."
    )

    return {
        "status": status,
        "field_training_eligible": False,
        "rejection_reasons": reasons
    }


def main():
    parser = argparse.ArgumentParser(
        description="Read-only Kraftek BMS AI dataset exporter"
    )
    parser.add_argument("--database", default=DEFAULT_DB)
    parser.add_argument(
        "--output",
        default=os.path.expanduser("~/kraftek_bms_exports")
    )
    parser.add_argument(
        "--purpose",
        choices=("archive", "field-training"),
        default="archive",
        help="Export purpose: archive or field-training"
    )
    args = parser.parse_args()

    if not os.path.isfile(args.database):
        print("ERROR: Database not found:", args.database)
        return 1

    timestamp = datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ")

    uri = "file:" + args.database + "?mode=ro"
    con = sqlite3.connect(uri, uri=True)

    try:
        con.execute("BEGIN")

        tables = [
            row[0] for row in con.execute("""
                SELECT name FROM sqlite_master
                WHERE type='table'
            """)
        ]

        required = [
            "telemetry", "block_readings",
            "ai_diagnostics", "events"
        ]

        missing = [name for name in required if name not in tables]
        if missing:
            raise RuntimeError(
                "Required database tables missing: " + ", ".join(missing)
            )

        telemetry_columns = [
            row[1] for row in
            con.execute("PRAGMA table_info(telemetry)")
        ]

        expected = [
            "id", "timestamp_utc", "data_source",
            "bank_voltage_v", "bank_current_a",
            "soc_pct", "soh_pct", "mode",
            "system_state", "data_quality", "scenario_label"
        ]

        missing_columns = [
            name for name in expected
            if name not in telemetry_columns
        ]
        if missing_columns:
            raise RuntimeError(
                "Telemetry columns missing: " +
                ", ".join(missing_columns)
            )

        counts = {}
        for table in required:
            counts[table] = con.execute(
                'SELECT COUNT(*) FROM "' + table + '"'
            ).fetchone()[0]

        quality_counts = con.execute("""
            SELECT data_source, data_quality, COUNT(*)
            FROM telemetry
            GROUP BY data_source, data_quality
            ORDER BY data_source, data_quality
        """).fetchall()

        provenance = assess_provenance(quality_counts)

        if args.purpose == "field-training" and not provenance[
            "field_training_eligible"
        ]:
            print("FIELD-TRAINING EXPORT REJECTED")
            print("Provenance status:", provenance["status"])
            for reason in provenance["rejection_reasons"]:
                print("-", reason)
            print("No export directory or files were created.")
            return 2

        final_dir = os.path.join(args.output, "export_" + timestamp)
        staging_dir = final_dir + ".tmp"

        os.makedirs(args.output, exist_ok=True)
        if os.path.exists(final_dir) or os.path.exists(staging_dir):
            print("ERROR: Export directory already exists.")
            print("Final:", final_dir)
            print("Staging:", staging_dir)
            return 1

        os.makedirs(staging_dir)
        output_dir = staging_dir

        telemetry_fields = [
            "id", "timestamp_utc", "data_source",
            "bank_voltage_v", "bank_current_a",
            "soc_pct", "soh_pct", "mode",
            "system_state", "data_quality", "scenario_label",
            "block_count"
        ]

        block_fields = [
            "voltage_v", "temperature_c",
            "soc_pct", "soh_pct", "sensor_valid"
        ]

        for block_id in range(1, 5):
            for field in block_fields:
                telemetry_fields.append(
                    "block_%d_%s" % (block_id, field)
                )

        expressions = []
        for block_id in range(1, 5):
            for field in block_fields:
                expressions.append(
                    "MAX(CASE WHEN b.block_id = %d "
                    "THEN b.%s END) AS block_%d_%s"
                    % (block_id, field, block_id, field)
                )

        telemetry_query = """
            SELECT
                t.id, t.timestamp_utc, t.data_source,
                t.bank_voltage_v, t.bank_current_a,
                t.soc_pct, t.soh_pct, t.mode,
                t.system_state, t.data_quality, t.scenario_label,
                COUNT(b.id) AS block_count,
                %s
            FROM telemetry t
            LEFT JOIN block_readings b
                ON b.telemetry_id = t.id
            GROUP BY t.id
            ORDER BY t.id
        """ % ", ".join(expressions)

        telemetry_count = write_csv(
            os.path.join(output_dir, "telemetry.csv"),
            telemetry_query,
            telemetry_fields,
            con
        )

        block_columns = [
            "id", "telemetry_id", "block_id",
            "voltage_v", "temperature_c",
            "soc_pct", "soh_pct", "sensor_valid"
        ]

        block_query = """
            SELECT id, telemetry_id, block_id,
                   voltage_v, temperature_c,
                   soc_pct, soh_pct, sensor_valid
            FROM block_readings
            ORDER BY telemetry_id, block_id
        """

        block_count = write_csv(
            os.path.join(output_dir, "block_readings.csv"),
            block_query, block_columns, con
        )

        diagnostic_columns = [
            "id", "telemetry_id", "timestamp_utc",
            "scenario_label", "status", "summary",
            "ml_class", "statistical_score", "ml_score",
            "model_version"
        ]

        diagnostic_query = """
            SELECT id, telemetry_id, timestamp_utc,
                   scenario_label, status, summary,
                   ml_class, statistical_score, ml_score,
                   model_version
            FROM ai_diagnostics
            ORDER BY id
        """

        diagnostic_count = write_csv(
            os.path.join(output_dir, "ai_diagnostics.csv"),
            diagnostic_query, diagnostic_columns, con
        )

        event_columns = [
            "id", "timestamp_utc", "severity",
            "event_code", "message", "source", "acknowledged"
        ]

        event_query = """
            SELECT id, timestamp_utc, severity,
                   event_code, message, source, acknowledged
            FROM events
            ORDER BY id
        """

        event_count = write_csv(
            os.path.join(output_dir, "events.csv"),
            event_query, event_columns, con
        )

        scenario_counts = con.execute("""
            SELECT scenario_label, COUNT(*)
            FROM telemetry
            GROUP BY scenario_label
            ORDER BY scenario_label
        """).fetchall()

        diagnostic_status = con.execute("""
            SELECT status, COUNT(*)
            FROM ai_diagnostics
            GROUP BY status
            ORDER BY status
        """).fetchall()

        model_versions = con.execute("""
            SELECT model_version, COUNT(*)
            FROM ai_diagnostics
            GROUP BY model_version
        """).fetchall()

        block_distribution = con.execute("""
            SELECT block_count, COUNT(*)
            FROM (
                SELECT t.id, COUNT(b.id) AS block_count
                FROM telemetry t
                LEFT JOIN block_readings b
                    ON b.telemetry_id = t.id
                GROUP BY t.id
            )
            GROUP BY block_count
            ORDER BY block_count
        """).fetchall()

        duplicate_pairs = con.execute("""
            SELECT COUNT(*) FROM (
                SELECT telemetry_id, block_id
                FROM block_readings
                GROUP BY telemetry_id, block_id
                HAVING COUNT(*) > 1
            )
        """).fetchone()[0]

        unmatched_diagnostics = con.execute("""
            SELECT COUNT(*)
            FROM ai_diagnostics d
            LEFT JOIN telemetry t ON t.id = d.telemetry_id
            WHERE t.id IS NULL
        """).fetchone()[0]

        time_range = con.execute("""
            SELECT MIN(timestamp_utc), MAX(timestamp_utc)
            FROM telemetry
        """).fetchone()

        manifest = {
            "export_timestamp_utc": timestamp,
            "database": args.database,
            "database_access": "READ_ONLY",
            "consistency": "Single SQLite read transaction",
            "provenance_note": (
                "Source and quality values are preserved as stored. "
                "Do not interpret VALID as proof of physical hardware "
                "measurements."
            ),
            "provenance_status": provenance["status"],
            "field_training_eligible": (
                provenance["field_training_eligible"]
            ),
            "field_training_rejection_reasons": (
                provenance["rejection_reasons"]
            ),
            "field_training_policy": (
                "FAIL_CLOSED_NO_HARDWARE_SOURCE_APPROVED"
            ),
            "diagnostic_label_note": (
                "AI diagnostic outputs are model predictions/advisories, "
                "not verified ground-truth fault labels."
            ),
            "time_range_utc": {
                "first": time_range[0],
                "last": time_range[1]
            },
            "source_table_counts": counts,
            "exported_row_counts": {
                "telemetry.csv": telemetry_count,
                "block_readings.csv": block_count,
                "ai_diagnostics.csv": diagnostic_count,
                "events.csv": event_count
            },
            "data_source_quality_counts": quality_counts,
            "scenario_counts": scenario_counts,
            "diagnostic_status_counts": diagnostic_status,
            "diagnostic_model_versions": model_versions,
            "block_count_distribution": block_distribution,
            "duplicate_snapshot_block_pairs": duplicate_pairs,
            "diagnostics_without_telemetry": unmatched_diagnostics,
            "telemetry_schema_columns": telemetry_columns,
            "files": [
                "telemetry.csv",
                "block_readings.csv",
                "ai_diagnostics.csv",
                "events.csv",
                "manifest.json"
            ]
        }

        manifest_path = os.path.join(output_dir, "manifest.json")
        with open(manifest_path, "w", encoding="utf-8") as handle:
            json.dump(manifest, handle, indent=2, ensure_ascii=False)
            handle.write("\n")

        os.rename(staging_dir, final_dir)
        output_dir = final_dir
        manifest_path = os.path.join(output_dir, "manifest.json")

        print("EXPORT COMPLETED")
        print("Purpose:", args.purpose)
        print("Provenance status:", provenance["status"])
        print("Field-training eligible:", provenance["field_training_eligible"])
        print("Output:", output_dir)
        print("Telemetry:", telemetry_count)
        print("Block readings:", block_count)
        print("AI diagnostics:", diagnostic_count)
        print("Events:", event_count)
        print("Block distribution:", block_distribution)
        print("Duplicate block pairs:", duplicate_pairs)
        print("Unmatched diagnostics:", unmatched_diagnostics)
        print("Manifest:", manifest_path)

        return 0

    finally:
        con.rollback()
        con.close()


if __name__ == "__main__":
    sys.exit(main())
