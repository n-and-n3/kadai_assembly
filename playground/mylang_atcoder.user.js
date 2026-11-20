// ==UserScript==
// @name         AtCoder MyLang
// @namespace    https://atcoder.jp/
// @version      0.1.1
// @description  Edit MyLang on AtCoder; test locally and submit generated GCC C++.
// @match        https://atcoder.jp/contests/*/tasks/*
// @match        https://atcoder.jp/contests/*/submit*
// @match        https://atcoder.jp/contests/*/custom_test*
// @grant        unsafeWindow
// @run-at       document-idle
// ==/UserScript==

(() => {
    "use strict";

    const SERVER = "http://127.0.0.1:8765/";
    const MODE_KEY = "atcoder-mylang-enabled";
    const contest = location.pathname.match(/^\/contests\/([^/]+)/)?.[1] || "unknown";
    const SOURCE_KEY = `atcoder-mylang-source:${contest}`;
    const page = unsafeWindow;
    let enabled = sessionStorage.getItem(MODE_KEY) === "1";
    let bypass = false;
    let temporaryCode = false;
    let busy = false;
    let sourceAtStart = sessionStorage.getItem(SOURCE_KEY);

    function showMode() {
        const button = document.getElementById("mylang-mode-button");
        if (button) {
            const label = enabled ? "MyLang ON" : "MyLang を使う";
            if (button.textContent !== label) button.textContent = label;
            button.classList.toggle("btn-success", enabled);
            button.classList.toggle("btn-default", !enabled);
        }
    }

    function editor() {
        const element = document.getElementById("editor");
        return element && page.ace ? page.ace.edit(element) : null;
    }

    function plain() {
        return document.getElementById("plain-textarea") || document.querySelector(".plain-textarea");
    }

    function sourceCode() {
        const e = editor();
        const text = plain();
        const plainActive = document.querySelector(".btn-toggle-editor.active");
        if (plainActive && text) return text.value;
        if (e) return e.getValue();
        if (text) return text.value;
        if (typeof page.getSourceCode === "function") return page.getSourceCode();
        throw new Error("AtCoder のソースコード欄が見つかりません");
    }

    function setSourceCode(value) {
        const e = editor();
        const text = plain();
        if (e) e.setValue(value, -1);
        if (text) text.value = value;
        if (!e && !text) throw new Error("AtCoder のソースコード欄が見つかりません");
    }

    function saveSource() {
        if (enabled && !temporaryCode) {
            try { sessionStorage.setItem(SOURCE_KEY, sourceCode()); } catch (_) { /* editor not ready */ }
        }
    }

    async function compile(source) {
        let response;
        try {
            response = await fetch(SERVER, {
                method: "POST",
                mode: "cors",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ mode: "compile", sourceCode: source }),
            });
        } catch (_) {
            throw new Error("localhost に接続できません。MyLang サーバーとブラウザのローカル接続許可を確認してください");
        }
        const body = await response.json();
        if (!response.ok || body.status !== "success") {
            throw new Error(body.stderr || body.error || `HTTP ${response.status}`);
        }
        return body;
    }

    function currentLanguageSelect() {
        return document.querySelector("#select-lang select.current") ||
               document.querySelector("#select-lang select");
    }

    function selectMyLang() {
        const select = currentLanguageSelect();
        const option = select?.querySelector("option[data-mylang]");
        if (!option) return;
        option.selected = true;
        select.dispatchEvent(new Event("change", { bubbles: true }));
        if (page.jQuery?.fn?.select2) page.jQuery(select).trigger("change.select2");
    }

    function chooseEasyTestRunner() {
        const select = document.getElementById("atcoder-easy-test-language");
        if (!select) return;
        const options = Array.from(select.options);
        if (!enabled && !/MyLang.*\[Local\]/.test(select.selectedOptions[0]?.textContent || "")) return;
        const option = enabled
            ? options.find(o => /MyLang.*\[Local\]/.test(o.textContent))
            : options.find(o => !/MyLang.*\[Local\]/.test(o.textContent));
        if (option && select.value !== option.value) {
            select.value = option.value;
            select.dispatchEvent(new Event("change", { bubbles: true }));
        }
    }

    function addLanguageOption(select) {
        if (select.querySelector("option[data-mylang]")) return;
        const cpp = Array.from(select.options).find(o => /C\+\+23.*GCC/i.test(o.textContent)) ||
                    Array.from(select.options).find(o => /C\+\+.*GCC/i.test(o.textContent));
        if (!cpp) return;
        const option = document.createElement("option");
        option.value = cpp.value; // AtCoder still receives a genuine GCC C++ language ID.
        option.textContent = `MyLang (${cpp.textContent.trim()})`;
        option.dataset.mylang = "1";
        select.appendChild(option);
        select.addEventListener("change", () => {
            enabled = Boolean(select.selectedOptions[0]?.dataset.mylang);
            sessionStorage.setItem(MODE_KEY, enabled ? "1" : "0");
            showMode();
            if (enabled) saveSource();
            chooseEasyTestRunner();
        });
        if (enabled) selectMyLang();
    }

    function installUI() {
        for (const select of document.querySelectorAll("#select-lang select")) addLanguageOption(select);
        chooseEasyTestRunner();
        const anchor = document.getElementById("select-lang");
        if (anchor && !document.getElementById("mylang-mode-button")) {
            const button = document.createElement("button");
            button.id = "mylang-mode-button";
            button.type = "button";
            button.className = "btn btn-sm";
            button.style.marginTop = "6px";
            button.addEventListener("click", () => {
                const next = !enabled;
                const select = currentLanguageSelect();
                const myLangOption = select?.querySelector("option[data-mylang]");
                const option = next ? myLangOption :
                    Array.from(select?.options || []).find(o => !o.dataset.mylang && o.value === myLangOption?.value);
                if (option) {
                    option.selected = true;
                    select.dispatchEvent(new Event("change", { bubbles: true }));
                    if (page.jQuery?.fn?.select2) page.jQuery(select).trigger("change.select2");
                }
                enabled = next;
                sessionStorage.setItem(MODE_KEY, enabled ? "1" : "0");
                showMode();
                if (enabled) saveSource();
                chooseEasyTestRunner();
            });
            anchor.insertAdjacentElement("afterend", button);
        }
        showMode();
        if (anchor && !document.getElementById("mylang-show-gas")) {
            const button = document.createElement("button");
            button.id = "mylang-show-gas";
            button.type = "button";
            button.className = "btn btn-default btn-sm";
            button.textContent = "生成 GAS を表示";
            button.style.marginTop = "6px";
            button.addEventListener("click", async () => {
                const tab = window.open("", "_blank");
                try {
                    if (!tab) throw new Error("ポップアップがブロックされました");
                    tab.document.body.textContent = "コンパイル中…";
                    const result = await compile(sourceCode());
                    tab.document.body.textContent = "";
                    const pre = tab.document.createElement("pre");
                    pre.textContent = result.assembly;
                    tab.document.body.appendChild(pre);
                } catch (error) {
                    if (tab) tab.close();
                    alert(`MyLang: ${error.message}`);
                }
            });
            anchor.insertAdjacentElement("afterend", button);
        }
    }

    function restoreEditorSoon(original) {
        setTimeout(() => {
            setSourceCode(original);
            temporaryCode = false;
            busy = false;
        }, 1000);
    }

    async function handleSubmit(button, form) {
        if (busy) return;
        busy = true;
        let original;
        try {
            original = sourceCode();
            saveSource();
            const { cpp } = await compile(original);
            if (location.pathname.includes("/custom_test")) {
                if (!button) throw new Error("コードテストの実行ボタンが見つかりません");
                // The built-in code-test handler reads the editor synchronously on click.
                temporaryCode = true;
                setSourceCode(cpp);
                bypass = true;
                button.click();
                bypass = false;
                restoreEditorSoon(original);
            } else if (form) {
                const field = form.querySelector('[name="sourceCode"]') || plain();
                if (!field) throw new Error("提出フォームの sourceCode 欄が見つかりません");
                field.value = cpp;
                HTMLFormElement.prototype.submit.call(form);
            } else {
                temporaryCode = true;
                setSourceCode(cpp);
                bypass = true;
                button.click();
                bypass = false;
                restoreEditorSoon(original);
            }
        } catch (error) {
            if (temporaryCode) setSourceCode(original);
            temporaryCode = false;
            bypass = false;
            alert(`MyLang の変換に失敗しました:\n${error.message}`);
        } finally {
            if (!temporaryCode) busy = false;
        }
    }

    document.addEventListener("click", event => {
        if (bypass || !enabled) return;
        const selector = location.pathname.includes("/custom_test")
            ? "#submit, .col-sm-5 > .btn-primary"
            : "#submit";
        const button = event.target.closest?.(selector);
        if (!button) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        handleSubmit(button, button.closest("form"));
    }, true);

    document.addEventListener("submit", event => {
        if (bypass || !enabled) return;
        const form = event.target;
        if (!form.querySelector("#select-lang") && !form.querySelector("#plain-textarea")) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        handleSubmit(form.querySelector("#submit") || form.querySelector('[type="submit"]'), form);
    }, true);

    document.addEventListener("input", event => {
        if (event.target === plain()) saveSource();
    }, true);

    let hookedEditor = null;
    const observer = new MutationObserver(() => {
        installUI();
        const e = editor();
        if (e && e !== hookedEditor) {
            hookedEditor = e;
            const enteringTestOrSubmit = /\/(custom_test|submit)(?:\/|$)/.test(location.pathname);
            if (enabled && sourceAtStart && (enteringTestOrSubmit || !e.getValue())) {
                setSourceCode(sourceAtStart);
                sourceAtStart = null;
            }
            e.getSession().on("change", saveSource);
        }
    });
    observer.observe(document.documentElement, { childList: true, subtree: true });
    installUI();
})();
