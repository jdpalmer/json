import glob
import json
import os
import pytest
import subprocess
from jsondiff import diff
from sys import platform

@pytest.mark.parametrize("fn", glob.glob("*.good.json"))
def test_good(fn):
    with open(fn, "r") as fh:
        original = json.load(fh)
    output = json.loads(subprocess.check_output(
        "./jsontest " + fn, shell=True).decode("utf-8").strip())
    assert diff(original, output) == {}

@pytest.mark.parametrize("fn", glob.glob("*.bad.json"))
def test_bad(fn):
    assert subprocess.call("./jsontest " + fn, shell=True) != 0

if platform == "darwin":
    @pytest.mark.parametrize("fn", glob.glob("*.json"))
    def test_leaks(fn):
        assert subprocess.call("leaks -atExit -- ./jsontest " + fn, shell=True) == 0
