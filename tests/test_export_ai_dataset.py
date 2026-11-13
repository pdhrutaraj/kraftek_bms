import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "scripts")
    )
)

from export_ai_dataset import assess_provenance


class ProvenanceTests(unittest.TestCase):

    def test_empty_dataset_is_rejected(self):
        result = assess_provenance([])
        self.assertEqual(result["status"], "NO_TELEMETRY")
        self.assertFalse(result["field_training_eligible"])

    def test_simulator_only_is_rejected(self):
        result = assess_provenance([
            ("SIMULATOR", "SIMULATED", 10)
        ])
        self.assertEqual(result["status"], "SIMULATOR_ONLY")
        self.assertFalse(result["field_training_eligible"])

    def test_valid_quality_does_not_approve_simulator(self):
        result = assess_provenance([
            ("SIMULATOR", "VALID", 10)
        ])
        self.assertEqual(result["status"], "SIMULATOR_ONLY")
        self.assertFalse(result["field_training_eligible"])

    def test_mixed_sources_are_rejected(self):
        result = assess_provenance([
            ("SIMULATOR", "VALID", 10),
            ("RXN_BMS", "VALID", 5)
        ])
        self.assertEqual(result["status"], "MIXED_PROVENANCE")
        self.assertFalse(result["field_training_eligible"])

    def test_rxn_placeholder_is_not_approved(self):
        result = assess_provenance([
            ("RXN_BMS", "VALID", 5)
        ])
        self.assertEqual(
            result["status"], "UNVERIFIED_SOURCE_PRESENT"
        )
        self.assertFalse(result["field_training_eligible"])
        self.assertTrue(
            any("placeholder" in reason.lower()
                for reason in result["rejection_reasons"])
        )

    def test_unknown_source_is_rejected(self):
        result = assess_provenance([
            ("UNRECOGNIZED_SOURCE", "VALID", 3)
        ])
        self.assertEqual(
            result["status"], "UNVERIFIED_SOURCE_PRESENT"
        )
        self.assertFalse(result["field_training_eligible"])

    def test_blank_source_is_rejected(self):
        result = assess_provenance([
            (None, "VALID", 2)
        ])
        self.assertEqual(
            result["status"], "UNVERIFIED_SOURCE_PRESENT"
        )
        self.assertFalse(result["field_training_eligible"])


if __name__ == "__main__":
    unittest.main()
