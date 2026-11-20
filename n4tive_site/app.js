"use strict";

const example = `main(){
    input n;
    print n * (n + 1) / 2;
}`;
const $ = id => document.getElementById(id);
const source = $("source");
const stdin = $("stdin");
const expected = $("expected");
const cpp = $("cpp");
let generatedFor = null;
let busy = false;

function loadSaved(key, fallback) {
    try { return localStorage.getItem(`n4tive:${key}`) ?? fallback; }
    catch { return fallback; }
}
function save(key, value) {
    try { localStorage.setItem(`n4tive:${key}`, value); } catch { /* storage may be disabled */ }
}
function sizeLabel() {
    $("source-size").textContent = `${new TextEncoder().encode(source.value).length} bytes`;
}
function invalidate() {
    generatedFor = null;
    cpp.value = "";
    $("copy-cpp").disabled = true;
    $("download-cpp").disabled = true;
    sizeLabel();
}
function setStatus(status, timeMs) {
    const labels = { idle: "未実行", working: "処理中…", ok: "生成完了", OK: "実行完了", AC: "AC", WA: "WA", RE: "RE", TLE: "TLE", OLE: "OLE", CE: "CE", error: "エラー" };
    const badge = $("status");
    badge.textContent = labels[status] ?? status;
    badge.className = `status ${status}`;
    $("time").textContent = Number.isFinite(timeMs) ? `${timeMs} ms` : "";
}
function showResult(result, run) {
    $("result-placeholder").hidden = true;
    $("result-details").hidden = false;
    setStatus(result.status, result.timeMs);
    $("stdout").textContent = run ? (result.stdout || "（出力なし）") : "（C++ を生成しました）";
    $("stderr").textContent = result.message || result.stderr || "（なし）";
    if (typeof result.cpp === "string") {
        generatedFor = source.value;
        cpp.value = result.cpp;
        $("copy-cpp").disabled = false;
        $("download-cpp").disabled = false;
    }
}
async function request(run) {
    if (busy) return;
    busy = true;
    $("run").disabled = true;
    $("compile").disabled = true;
    setStatus("working");
    const submittedSource = source.value;
    try {
        const response = await fetch(run ? "/api/run" : "/api/compile", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ source: submittedSource, stdin: stdin.value, expected: expected.value }),
        });
        const result = await response.json();
        if (source.value !== submittedSource) {
            showResult({ status: "error", message: "実行中にソースが変わりました。もう一度実行してください。" }, run);
            return;
        }
        showResult(result, run);
    } catch (error) {
        showResult({ status: "error", message: `サーバーへ接続できません: ${error.message}` }, run);
    } finally {
        busy = false;
        $("run").disabled = false;
        $("compile").disabled = false;
    }
}
function download(filename, contents) {
    const blob = new Blob([contents], { type: "text/plain;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename;
    link.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}
async function copyCpp() {
    if (generatedFor !== source.value || !cpp.value) return;
    try {
        await navigator.clipboard.writeText(cpp.value);
        const button = $("copy-cpp");
        button.textContent = "コピーしました";
        setTimeout(() => { button.textContent = "コピー"; }, 1800);
    } catch {
        cpp.select();
        setStatus("error");
        $("stderr").textContent = "自動コピーが使えません。選択された C++ を手動でコピーしてください。";
    }
}

source.value = loadSaved("source", example);
stdin.value = loadSaved("stdin", "5\n");
expected.value = loadSaved("expected", "15\n");
sizeLabel();

source.addEventListener("input", () => { save("source", source.value); invalidate(); });
stdin.addEventListener("input", () => save("stdin", stdin.value));
expected.addEventListener("input", () => save("expected", expected.value));
source.addEventListener("keydown", event => {
    if (event.key === "Tab") {
        event.preventDefault();
        const start = source.selectionStart;
        source.setRangeText("    ", start, source.selectionEnd, "end");
        source.dispatchEvent(new Event("input"));
    }
});
document.addEventListener("keydown", event => {
    if (event.key === "Enter" && (event.ctrlKey || event.metaKey)) {
        event.preventDefault();
        request(true);
    }
});
$("run").addEventListener("click", () => request(true));
$("compile").addEventListener("click", () => request(false));
$("copy-cpp").addEventListener("click", copyCpp);
$("download-cpp").addEventListener("click", () => {
    if (generatedFor === source.value) download("Main.cpp", cpp.value);
});
$("download-n4").addEventListener("click", () => download("main.n4", source.value));
$("load-example").addEventListener("click", () => {
    if (source.value !== example && !confirm("現在のソースを例に置き換えますか？")) return;
    source.value = example;
    stdin.value = "5\n";
    expected.value = "15\n";
    save("source", source.value);
    save("stdin", stdin.value);
    save("expected", expected.value);
    invalidate();
    source.focus();
});
