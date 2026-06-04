(function () {
  function renderMermaid() {
    if (!window.mermaid) {
      return;
    }

    window.mermaid.initialize({
      startOnLoad: false,
      securityLevel: "strict",
      theme: "default"
    });

    var diagrams = Array.prototype.slice.call(document.querySelectorAll(".mermaid"));
    var pending = diagrams.filter(function (diagram) {
      return diagram.getAttribute("data-processed") !== "true";
    });

    if (pending.length === 0) {
      return;
    }

    pending.forEach(function (diagram) {
      diagram.setAttribute("data-processed", "true");
    });

    if (typeof window.mermaid.run === "function") {
      window.mermaid.run({ nodes: pending });
      return;
    }

    window.mermaid.init(undefined, pending);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", renderMermaid);
  } else {
    renderMermaid();
  }

  if (typeof document$ !== "undefined" && document$.subscribe) {
    document$.subscribe(renderMermaid);
  }
})();
