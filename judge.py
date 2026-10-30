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
    ["gcc","-Wall","-Wextra","-g","-O0",calcPath,"-o","calc0.out"],
    capture_output=True,
    text=True,
)
print(result)

testPath = Path('.')/"testcases"

for item in testPath.iterdir():
    content = item.read_text(encoding="utf-8")
    testcases = content.split("\n")
    for testcase in testcases:
        try:
            test_in, test_out = testcase.split(",")
        except:
            print("Can't parse testcase:", testcase)
        test_out.strip()
        result = subprocess.run(
            ["./calc0.out", test_in],
            capture_output=True,
            text=True,
            timeout=3
        ).stdout.strip()
        #print([ord(c) for c in result], [ord(c) for c in test_out])
        if result != test_out:
            print("Hacked!")
            print("input:",test_in)
            print("expexcted:",test_out)
            print("output:",result)
            assert False
        else:
            print("passed :",test_in,result)







