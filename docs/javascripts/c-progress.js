/*
 * C learning progress.
 *
 * On every page of the C section, record the visit date. On the progress page
 * (docs/c/progress.md), turn each listed topic into a "learnt" checkbox, show
 * visit dates and per-chapter counts, and offer export, import, and reset.
 *
 * State lives in this browser's localStorage only. Without JavaScript or
 * storage, the progress page is still a plain list of links.
 */
(function () {
  "use strict";

  var STORE_KEY = "c-learning-progress-v1";

  function emptyState() {
    return { learnt: {}, visited: {} };
  }

  function load() {
    try {
      var state = JSON.parse(window.localStorage.getItem(STORE_KEY));
      if (state && typeof state === "object") {
        state.learnt = state.learnt || {};
        state.visited = state.visited || {};
        return state;
      }
    } catch (error) {
      /* storage unavailable or corrupt: start empty */
    }
    return emptyState();
  }

  function save(state) {
    try {
      window.localStorage.setItem(STORE_KEY, JSON.stringify(state));
      return true;
    } catch (error) {
      return false;
    }
  }

  /* Same key for "/c/x/", "/c/x/index.html", and "/c/x" (directory URLs or files). */
  function pageKey(href) {
    var path = new URL(href, window.location.href).pathname;
    path = path.replace(/index\.html$/, "").replace(/\.html$/, "/");
    return path.charAt(path.length - 1) === "/" ? path : path + "/";
  }

  function today() {
    return new Date().toISOString().slice(0, 10);
  }

  function recordVisit() {
    var key = pageKey(window.location.href);
    if (!/\/c\//.test(key) || document.querySelector(".c-progress")) {
      return; /* outside the C section, or the progress page itself */
    }
    var state = load();
    state.visited[key] = today();
    save(state);
  }

  function element(tag, className, text) {
    var node = document.createElement(tag);
    if (className) {
      node.className = className;
    }
    if (text) {
      node.textContent = text;
    }
    return node;
  }

  function buildChecklist(lists) {
    var state = load();
    var items = [];

    lists.forEach(function (list) {
      /* The list sits in a <div class="c-progress">; its chapter heading is the
         nearest <h2> before that wrapper. */
      var heading = list.closest(".c-progress").previousElementSibling;
      while (heading && heading.tagName !== "H2") {
        heading = heading.previousElementSibling;
      }
      var chapter = { heading: heading, items: [] };
      if (heading) {
        chapter.counter = element("span", "c-progress-count");
        heading.insertBefore(chapter.counter, heading.querySelector(".headerlink"));
      }

      list.querySelectorAll("li").forEach(function (li) {
        var anchor = li.querySelector("a");
        if (!anchor) {
          return;
        }
        var key = pageKey(anchor.href);
        var box = document.createElement("input");
        box.type = "checkbox";
        box.className = "c-progress-box";
        box.setAttribute("aria-label", "Learnt: " + anchor.textContent);
        box.checked = Boolean(state.learnt[key]);

        var visited = element("span", "c-progress-visited");
        li.insertBefore(box, li.firstChild);
        li.appendChild(visited);

        var item = { key: key, box: box, li: li, visited: visited };
        box.addEventListener("change", function () {
          var current = load();
          if (box.checked) {
            current.learnt[key] = today();
          } else {
            delete current.learnt[key];
          }
          save(current);
          refresh();
        });
        chapter.items.push(item);
        items.push(item);
      });
      list.chapter = chapter;
    });

    var summary = document.getElementById("c-progress-summary");
    var totals = element("p", "c-progress-totals");
    var bar = element("div", "c-progress-bar");
    var fill = element("div", "c-progress-bar-fill");
    bar.appendChild(fill);
    bar.setAttribute("role", "progressbar");
    bar.setAttribute("aria-valuemin", "0");
    bar.setAttribute("aria-valuemax", String(items.length));

    var controls = element("div", "c-progress-controls");
    var hideLabel = element("label", "c-progress-hide");
    var hideBox = document.createElement("input");
    hideBox.type = "checkbox";
    hideLabel.appendChild(hideBox);
    hideLabel.appendChild(document.createTextNode(" Hide learnt topics"));
    var exportButton = element("button", "c-progress-button", "Export");
    var importButton = element("button", "c-progress-button", "Import");
    var resetButton = element("button", "c-progress-button", "Reset");
    var importInput = document.createElement("input");
    importInput.type = "file";
    importInput.accept = "application/json,.json";
    importInput.hidden = true;
    [exportButton, importButton, resetButton].forEach(function (button) {
      button.type = "button";
    });
    controls.appendChild(hideLabel);
    controls.appendChild(exportButton);
    controls.appendChild(importButton);
    controls.appendChild(resetButton);
    controls.appendChild(importInput);

    if (summary) {
      summary.appendChild(totals);
      summary.appendChild(bar);
      summary.appendChild(controls);
    }

    function refresh() {
      var current = load();
      var learnt = 0;
      var visited = 0;
      items.forEach(function (item) {
        var isLearnt = Boolean(current.learnt[item.key]);
        var visitDate = current.visited[item.key];
        item.box.checked = isLearnt;
        item.li.classList.toggle("c-progress-learnt", isLearnt);
        item.li.hidden = isLearnt && hideBox.checked;
        item.visited.textContent = isLearnt
          ? "learnt " + current.learnt[item.key]
          : visitDate ? "visited " + visitDate : "";
        learnt += isLearnt ? 1 : 0;
        visited += visitDate || isLearnt ? 1 : 0;
      });
      lists.forEach(function (list) {
        var chapter = list.chapter;
        if (chapter.counter) {
          var done = chapter.items.filter(function (item) {
            return item.box.checked;
          }).length;
          chapter.counter.textContent = done + " / " + chapter.items.length;
          chapter.counter.classList.toggle("c-progress-complete",
                                           done === chapter.items.length);
        }
      });
      totals.textContent = "Learnt " + learnt + " of " + items.length + " topics" +
        " · visited or learnt " + visited +
        " · not started " + (items.length - visited);
      fill.style.width = (items.length ? (100 * learnt / items.length) : 0) + "%";
      bar.setAttribute("aria-valuenow", String(learnt));
    }

    hideBox.addEventListener("change", refresh);

    exportButton.addEventListener("click", function () {
      var blob = new Blob([JSON.stringify(load(), null, 2)], { type: "application/json" });
      var link = document.createElement("a");
      link.href = URL.createObjectURL(blob);
      link.download = "c-learning-progress-" + today() + ".json";
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(link.href);
    });

    importButton.addEventListener("click", function () {
      importInput.click();
    });

    importInput.addEventListener("change", function () {
      var file = importInput.files && importInput.files[0];
      if (!file) {
        return;
      }
      file.text().then(function (text) {
        var incoming = JSON.parse(text);
        var current = load();
        ["learnt", "visited"].forEach(function (field) {
          Object.keys(incoming[field] || {}).forEach(function (key) {
            current[field][key] = incoming[field][key];
          });
        });
        save(current);
        refresh();
      }).catch(function () {
        window.alert("That file is not a progress export.");
      });
      importInput.value = "";
    });

    resetButton.addEventListener("click", function () {
      if (window.confirm("Clear all C learning progress stored in this browser?")) {
        save(emptyState());
        refresh();
      }
    });

    if (!save(load())) {
      totals.textContent = "This browser does not allow local storage, so progress " +
        "cannot be saved here.";
      return;
    }
    refresh();
  }

  function start() {
    recordVisit();
    var lists = Array.prototype.slice.call(document.querySelectorAll(".c-progress ul"));
    if (lists.length) {
      buildChecklist(lists);
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", start);
  } else {
    start();
  }
})();
