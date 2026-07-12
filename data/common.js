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
  return `//${window.location.host}${path}`;
}

async function getJson(path) {
  const response = await fetch(apiUrl(path));
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }

  return response.json();
}

async function postJson(path, payload) {
  const response = await fetch(apiUrl(path), {
    method: "POST",
    body: JSON.stringify(payload),
    headers: { "Content-type": "application/json; charset=UTF-8" },
  });

  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }
}
