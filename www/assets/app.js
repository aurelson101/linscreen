const year = document.querySelector("#year");
if (year) {
  year.textContent = new Date().getFullYear();
}

document.querySelectorAll("[data-copy-target]").forEach((button) => {
  button.addEventListener("click", async () => {
    const target = document.querySelector(button.dataset.copyTarget);
    if (!target) {
      return;
    }

    const original = button.innerHTML;
    try {
      await navigator.clipboard.writeText(target.textContent.trim());
      button.innerHTML = '<i class="bi bi-check2"></i> Copied';
      setTimeout(() => {
        button.innerHTML = original;
      }, 1600);
    } catch {
      button.innerHTML = '<i class="bi bi-exclamation-triangle"></i> Copy failed';
      setTimeout(() => {
        button.innerHTML = original;
      }, 1600);
    }
  });
});
