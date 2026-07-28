function toHex2(number) {
  return Number(number).toString(16).toUpperCase().padStart(2, "0");
}

function setPageError(message) {
  const errorElement = document.getElementById("error");
  if (errorElement) {
    errorElement.innerText = message || "";
  }
}

function apiUrl(path) {
  const cleanPath = path.startsWith("/") ? path : `/${path}`;

  if (window.location.protocol === "http:" || window.location.protocol === "https:") {
    return `${window.location.origin}${cleanPath}`;
  }

  const params = new URLSearchParams(window.location.search);
  const hostFromQuery = params.get("host");
  if (hostFromQuery) {
    localStorage.setItem("apiHost", hostFromQuery);
  }

  const storedHost = localStorage.getItem("apiHost");
  if (storedHost) {
    return `http://${storedHost}${cleanPath}`;
  }

  throw new Error("No API host available. Open this page from /page/... on the controller, or add ?host=<controller-ip>.");
}

async function getJson(path) {
  const url = apiUrl(path);
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status} for ${url}`);
  }

  return response.json();
}

async function postJson(path, payload) {
  const url = apiUrl(path);
  const response = await fetch(url, {
    method: "POST",
    body: JSON.stringify(payload),
    headers: { "Content-type": "application/json; charset=UTF-8" },
  });

  if (!response.ok) {
    throw new Error(`HTTP ${response.status} for ${url}`);
  }
}
