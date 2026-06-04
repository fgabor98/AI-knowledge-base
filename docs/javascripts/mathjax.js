window.MathJax = {
  tex: {
    inlineMath: [["\\(", "\\)"]],
    displayMath: [["\\[", "\\]"]],
    processEscapes: true,
    processEnvironments: true,
    tags: "ams"
  },
  options: {
    ignoreHtmlClass: ".*|",
    processHtmlClass: "arithmatex"
  }
};

(function () {
  function typesetMath() {
    if (!window.MathJax || !window.MathJax.typesetPromise) {
      return;
    }

    window.MathJax.startup.output.clearCache();
    window.MathJax.typesetClear();
    window.MathJax.texReset();
    window.MathJax.typesetPromise();
  }

  if (typeof document$ !== "undefined" && document$.subscribe) {
    document$.subscribe(typesetMath);
  }
})();
