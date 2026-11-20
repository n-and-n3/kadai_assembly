#!/usr/bin/env python3
"""Local compiler/runner for the MyLang AtCoder userscript and Easy Test v2."""

import argparse
import json
import subprocess
import sys
import tempfile
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "25B31026" / "codes"))
from gas_to_cpp import package_sources  # noqa: E402


COMPILER_SOURCE = ROOT / "25B31026" / "calc4.c"
MAX_REQUEST = 1_000_000
MAX_SOURCE = 100_000  # calc4 currently receives the source as one argv element.


def run_command(args, *, stdin=None, timeout=10):
    return subprocess.run(args, input=stdin, capture_output=True, text=True, errors="replace",
                          timeout=timeout)


class Handler(BaseHTTPRequestHandler):
    compiler: Path

    def do_GET(self):
        if self.path == "/":
            self.respond(200, {"status": "ready", "compiler": "MyLang"})
        else:
            self.respond(404, {"error": "not found"})

    def cors(self):
        self.send_header("Access-Control-Allow-Origin", "https://atcoder.jp")
        self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Vary", "Origin")

    def respond(self, status, body):
        payload = json.dumps(body, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.cors()
        self.end_headers()
        self.wfile.write(payload)

    def do_OPTIONS(self):
        self.send_response(204)
        self.cors()
        self.end_headers()

    def do_POST(self):
        origin = self.headers.get("Origin")
        if origin and origin != "https://atcoder.jp":
            self.respond(403, {"error": "origin not allowed"})
            return
        if self.path != "/":
            self.respond(404, {"error": "not found"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > MAX_REQUEST:
                self.respond(413, {"error": "invalid request size"})
                return
            request = json.loads(self.rfile.read(length))
            mode = request.get("mode")
            if mode == "list":
                self.respond(200, [{"language": "C++", "compilerName": "mylang",
                                    "label": "MyLang"}])
                return
            if mode not in ("compile", "run"):
                self.respond(400, {"error": "unknown mode"})
                return
            if mode == "run" and request.get("compilerName") != "mylang":
                self.respond(400, {"error": "unknown compiler"})
                return
            source = request.get("sourceCode")
            if not isinstance(source, str) or not source or len(source.encode()) > MAX_SOURCE or "\0" in source:
                self.respond(400, {"error": "invalid or too large sourceCode"})
                return
            stdin = request.get("stdin", "")
            if not isinstance(stdin, str) or len(stdin.encode()) > MAX_REQUEST:
                self.respond(400, {"error": "invalid stdin"})
                return
            with tempfile.TemporaryDirectory(prefix="mylang-atcoder-") as directory:
                work = Path(directory)
                generated = run_command([str(self.compiler), source], timeout=5)
                if generated.returncode != 0:
                    self.respond(200, {"status": "compileError", "stderr": generated.stderr})
                    return
                assembly = work / "program.s"
                assembly.write_text(generated.stdout, encoding="utf-8")
                try:
                    cpp = package_sources([assembly], original_source=source)
                except ValueError as error:
                    self.respond(200, {"status": "compileError", "stderr": str(error)})
                    return
                if mode == "compile":
                    self.respond(200, {"status": "success", "assembly": generated.stdout,
                                       "cpp": cpp})
                    return
                cpp_file = work / "Main.cpp"
                executable = work / "main"
                cpp_file.write_text(cpp, encoding="utf-8")
                built = run_command(["g++", "-std=gnu++23", "-O2", str(cpp_file),
                                     "-o", str(executable)], timeout=20)
                if built.returncode != 0:
                    self.respond(200, {"status": "compileError", "stderr": built.stderr})
                    return
                start = time.perf_counter()
                try:
                    result = run_command([str(executable)], stdin=stdin, timeout=3)
                except subprocess.TimeoutExpired:
                    self.respond(200, {"status": "success", "exitCode": 124,
                                       "time": 3000, "stdout": "", "stderr": "Time limit exceeded"})
                    return
                self.respond(200, {"status": "success", "exitCode": result.returncode,
                                   "time": int((time.perf_counter() - start) * 1000),
                                   "stdout": result.stdout, "stderr": result.stderr})
        except (json.JSONDecodeError, TypeError, ValueError) as error:
            self.respond(400, {"error": str(error)})
        except subprocess.TimeoutExpired:
            self.respond(200, {"status": "internalError", "stderr": "compiler timed out"})
        except Exception as error:
            self.respond(500, {"status": "internalError", "stderr": str(error)})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="mylang-compiler-") as directory:
        Handler.compiler = Path(directory) / "calc4"
        built = run_command(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                             str(COMPILER_SOURCE), "-o", str(Handler.compiler)], timeout=20)
        if built.returncode != 0:
            parser.exit(1, built.stderr)
        server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
        print(f"MyLang server: http://127.0.0.1:{args.port}/", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            server.server_close()


if __name__ == "__main__":
    main()
