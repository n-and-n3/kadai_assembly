import subprocess
import os
from pathlib import Path


fileName = "calc0.c"
calcPath = Path(".")/"25B31026"/fileName
#print(calcPath)

assert calcPath.exists()

content = calcPath.read_text(encoding="utf-8")
#print(content)
result = subprocess.run(
    ["gcc","-Wall","-Wextra","-g","-O0",calcPath,"-o","a.out"],
    capture_output=True,
    text=True,
)

testPath = Path('.')/"testcases"

for item in testPath.iterdir():
    content = item.read_text(encoding="utf-8")
    testcases = content.split("\n")
    for testcase in testcases:
        test_in, test_out = testcase.split(",")
        result = subprocess.run(
            ["./a.out"],
            input=test_in,
            capture_output=True,
            text=True,
            timeout=3
        ).stdout
        print(result, test_out)
        assert result == test_out







