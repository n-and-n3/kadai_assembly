"use strict";

document.querySelectorAll(".try-example").forEach(button => {
    button.addEventListener("click", () => {
        const code = button.closest(".example-card")?.querySelector("pre code")?.textContent;
        if (!code) return;
        try {
            localStorage.setItem("n4tive:source", code.trim());
            localStorage.setItem("n4tive:stdin", button.dataset.stdin ?? "");
            localStorage.setItem("n4tive:expected", button.dataset.expected ?? "");
            window.location.assign("/");
        } catch {
            alert("ブラウザの保存領域が使えません。コードをコピーしてコードテスト画面に貼り付けてください。");
        }
    });
});
