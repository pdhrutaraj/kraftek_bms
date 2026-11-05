cd ~/kraftek_bms

export QT_ROOT="$HOME/opt/qt6/6.2.4"
export PATH="$QT_ROOT/bin:$PATH"
export LD_LIBRARY_PATH="$QT_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

ctest --test-dir build_tx2nx --output-on-failure
