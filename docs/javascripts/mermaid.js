document.addEventListener("DOMContentLoaded", function () {
  if (typeof mermaid === "undefined") {
    return;
  }

  mermaid.initialize({
    startOnLoad: false,
    securityLevel: "strict",
    theme: "default"
  });

  document.querySelectorAll("pre.mermaid").forEach(function (diagram) {
    var container = document.createElement("div");
    container.className = "mermaid-container";
    diagram.parentNode.insertBefore(container, diagram);
    container.appendChild(diagram);

    var code = diagram.querySelector("code");
    if (code) {
      diagram.textContent = code.textContent;
    }
  });

  mermaid.run({ querySelector: ".mermaid" });
});
