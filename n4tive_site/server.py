#!/usr/bin/env python3
"""Small n4tive editor and code-test server for local use and NeoShowcase."""

import json
import os
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import BoundedSemaphore

ROOT = Path(__file__).resolve().parent.parent
SITE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "25B31026" / "codes"))
from gas_to_cpp import package_sources  # noqa: E402

MAX_REQUEST = 220_000
MAX_SOURCE = 100_000  # calc4 receives the source as one argv argument.
MAX_STDIN = 64_000
MAX_OUTPUT = 64_000
RUN_TIMEOUT = 3
SLOTS = BoundedSemaphore(2)


@dataclass
class Result:
    code: int
    stdout: str
    stderr: str
    elapsed_ms: int
    timed_out: bool = False
    stdout_truncated: bool = False


def limited(memory_bytes: int, cpu_seconds: int) -> None:
    resource.setrlimit(resource.RLIMIT_AS, (memory_bytes, memory_bytes))
    resource.setrlimit(resource.RLIMIT_CPU, (cpu_seconds, cpu_seconds))
    resource.setrlimit(resource.RLIMIT_FSIZE, (1_000_000, 1_000_000))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def execute(args: list[str], work: Path, phase: str, *, stdin: str = "",
            timeout: int = 10, memory_mb: int = 512,
            stdout_limit: int = MAX_OUTPUT) -> Result:
    output_path = work / f"{phase}.stdout"
    error_path = work / f"{phase}.stderr"
    started = time.perf_counter()
    with output_path.open("wb") as output, error_path.open("wb") as error:
        process = subprocess.Popen(
            args, stdin=subprocess.PIPE, stdout=output, stderr=error,
            start_new_session=True,
            preexec_fn=lambda: limited(memory_mb * 1024 * 1024, max(timeout, 1) + 1),
        )
        timed_out = False
        try:
            process.communicate(stdin.encode("utf-8"), timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
    stdout_bytes = output_path.read_bytes()
    return Result(
        process.returncode,
        stdout_bytes[:stdout_limit].decode("utf-8", "replace"),
        error_path.read_bytes()[:MAX_OUTPUT].decode("utf-8", "replace"),
        round((time.perf_counter() - started) * 1000),
        timed_out,
        len(stdout_bytes) > stdout_limit,
    )


def sandbox_command(program: Path, *args: str) -> list[str]:
    command = [
        "bwrap", "--unshare-user", "--unshare-pid", "--unshare-net",
        "--unshare-ipc", "--unshare-uts", "--die-with-parent", "--new-session",
        "--clearenv", "--setenv", "PATH", "/usr/bin:/bin",
        "--ro-bind", "/usr", "/usr",
    ]
    for directory in ("/lib", "/lib64"):
        if Path(directory).exists():
            command += ["--ro-bind", directory, directory]
    command += [
        "--proc", "/proc", "--dev", "/dev", "--tmpfs", "/tmp",
        "--ro-bind", str(program), "/program", "--chdir", "/",
        "/program", *args,
    ]
    return command


def sandbox_works(work: Path) -> tuple[bool, str]:
    if not shutil.which("bwrap"):
        return False, "bubblewrap (bwrap) がインストールされていません。"
    try:
        result = execute(sandbox_command(Path("/usr/bin/true")), work, "probe", timeout=2)
        if result.code == 0:
            return True, ""
        return False, result.stderr.strip() or f"bwrap が終了コード {result.code} で失敗しました。"
    except (OSError, subprocess.SubprocessError) as error:
        return False, str(error)


class Application:
    def __init__(self, compiler: Path, sandbox_ready: bool, sandbox_error: str):
        self.compiler = compiler
        self.sandbox_ready = sandbox_ready
        self.sandbox_error = sandbox_error

    def process(self, request: dict, run: bool) -> dict:
        source = request.get("source")
        stdin = request.get("stdin", "")
        expected = request.get("expected", "")
        if not isinstance(source, str) or not source.strip() or "\0" in source or len(source.encode()) > MAX_SOURCE:
            return {"status": "error", "message": "ソースは1～100,000バイトで指定してください。"}
        if not isinstance(stdin, str) or len(stdin.encode()) > MAX_STDIN:
            return {"status": "error", "message": "標準入力が長すぎます。"}
        if not isinstance(expected, str) or len(expected.encode()) > MAX_STDIN:
            return {"status": "error", "message": "期待する出力が長すぎます。"}
        if not self.sandbox_ready:
            return {"status": "error", "message":
                    "実行環境の隔離が利用できません。NeoShowcase の実行権限を確認してください。"
                    f" 詳細: {self.sandbox_error}"}

        with tempfile.TemporaryDirectory(prefix="n4tive-") as temp:
            work = Path(temp)
            generated = execute(sandbox_command(self.compiler, source), work, "n4tive",
                                timeout=5, stdout_limit=900_000)
            if generated.timed_out:
                return {"status": "CE", "message": "n4tive コンパイラが時間切れになりました。"}
            if generated.stdout_truncated:
                return {"status": "CE", "message": "生成されたアセンブリが大きすぎます。"}
            if generated.code != 0:
                return {"status": "CE", "message": generated.stderr or "n4tive のコンパイルに失敗しました。"}
            assembly = work / "program.s"
            assembly.write_text(generated.stdout, encoding="utf-8")
            try:
                cpp = package_sources([assembly], original_source=source)
            except ValueError as error:
                return {"status": "CE", "message": str(error)}

            cpp_path = work / "Main.cpp"
            executable = work / "main"
            cpp_path.write_text(cpp, encoding="utf-8")
            built = execute(["g++", "-std=gnu++23", "-O2", str(cpp_path), "-o", str(executable)],
                            work, "cpp", timeout=20, memory_mb=1536)
            if built.timed_out:
                return {"status": "CE", "message": "C++ のビルドが時間切れになりました。"}
            if built.code != 0:
                return {"status": "CE", "message": built.stderr or "生成した C++ をビルドできませんでした。"}
            if not run:
                return {"status": "ok", "cpp": cpp, "assembly": generated.stdout}

            executed = execute(sandbox_command(executable), work, "run", stdin=stdin,
                               timeout=RUN_TIMEOUT, memory_mb=256)
            response = {
                "status": "TLE" if executed.timed_out else "OLE" if executed.stdout_truncated else "RE" if executed.code != 0 else "OK",
                "stdout": executed.stdout,
                "stderr": executed.stderr,
                "exitCode": executed.code,
                "timeMs": executed.elapsed_ms,
                "cpp": cpp,
            }
            if expected.strip() and response["status"] == "OK":
                response["status"] = "AC" if executed.stdout.split() == expected.split() else "WA"
            return response


class Server(ThreadingHTTPServer):
    daemon_threads = True
    app: Application


class Handler(BaseHTTPRequestHandler):
    def send_data(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; base-uri 'none'; frame-ancestors 'none'")
        self.end_headers()
        self.wfile.write(body)

    def send_json(self, status: int, value: dict) -> None:
        self.send_data(status, json.dumps(value, ensure_ascii=False).encode("utf-8"),
                       "application/json; charset=utf-8")

    def do_GET(self) -> None:
        if self.path == "/healthz":
            self.send_json(200 if self.server.app.sandbox_ready else 503,
                           {"status": "ok" if self.server.app.sandbox_ready else "sandbox unavailable",
                            "detail": self.server.app.sandbox_error})
            return
        files = {"/": ("index.html", "text/html; charset=utf-8"),
                 "/guide": ("guide.html", "text/html; charset=utf-8"),
                 "/app.js": ("app.js", "text/javascript; charset=utf-8"),
                 "/guide.js": ("guide.js", "text/javascript; charset=utf-8"),
                 "/style.css": ("style.css", "text/css; charset=utf-8"),
                 "/guide.css": ("guide.css", "text/css; charset=utf-8")}
        if self.path not in files:
            self.send_json(404, {"status": "error", "message": "Not found"})
            return
        name, content_type = files[self.path]
        self.send_data(200, (SITE / name).read_bytes(), content_type)

    def do_POST(self) -> None:
        if self.path not in ("/api/compile", "/api/run"):
            self.send_json(404, {"status": "error", "message": "Not found"})
            return
        if self.headers.get("Content-Type", "").split(";")[0].strip() != "application/json":
            self.send_json(415, {"status": "error", "message": "JSON を送信してください。"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length < 1 or length > MAX_REQUEST:
                self.send_json(413, {"status": "error", "message": "リクエストが大きすぎます。"})
                return
            request = json.loads(self.rfile.read(length))
            if not isinstance(request, dict):
                raise ValueError("JSON object required")
        except (ValueError, json.JSONDecodeError):
            self.send_json(400, {"status": "error", "message": "JSON を読めませんでした。"})
            return
        if not SLOTS.acquire(blocking=False):
            self.send_json(429, {"status": "error", "message": "実行枠が埋まっています。少し待ってください。"})
            return
        try:
            result = self.server.app.process(request, self.path == "/api/run")
            self.send_json(200, result)
        except (OSError, subprocess.SubprocessError) as error:
            self.send_json(500, {"status": "error", "message": f"サーバーの実行エラー: {error}"})
        finally:
            SLOTS.release()


def main() -> None:
    port = int(os.environ.get("PORT", "8000"))
    host = os.environ.get("HOST", "127.0.0.1")
    with tempfile.TemporaryDirectory(prefix="n4tive-server-") as temp:
        work = Path(temp)
        compiler = Path(os.environ.get("N4TIVE_COMPILER", str(work / "calc4")))
        if "N4TIVE_COMPILER" not in os.environ:
            built = execute(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                             str(ROOT / "25B31026" / "calc4.c"), "-o", str(compiler)],
                            work, "build-compiler", timeout=20, memory_mb=1024)
            if built.code != 0 or built.timed_out:
                raise SystemExit(built.stderr or "n4tive コンパイラをビルドできませんでした。")
        ready, sandbox_error = sandbox_works(work)
        server = Server((host, port), Handler)
        server.app = Application(compiler, ready, sandbox_error)
        print(f"n4tive: http://{host}:{port}/ (sandbox: {'ready' if ready else 'unavailable'})", flush=True)
        if not ready:
            print(f"n4tive: sandbox probe failed: {sandbox_error}", file=sys.stderr, flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            server.server_close()


if __name__ == "__main__":
    main()
