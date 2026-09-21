import { API_BASE_URL } from '../config/api';

export class ServerDownError extends Error {
  constructor(message = 'Server is unreachable') {
    super(message);
    this.name = 'ServerDownError';
  }
}

// Global listener for server unreachability / fault
let onServerDownHandler: (() => void) | null = null;

export function registerServerDownHandler(handler: () => void) {
  onServerDownHandler = handler;
}

export function triggerServerDown() {
  if (onServerDownHandler) {
    onServerDownHandler();
  }
}

/**
 * Pings the server health/status endpoint.
 * Returns true if server responds with 2xx status, false otherwise.
 */
export async function pingServer(timeoutMs = 3500): Promise<boolean> {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);

  try {
    const res = await fetch(`${API_BASE_URL}/api/auth/status`, {
      method: 'GET',
      signal: controller.signal,
    });
    clearTimeout(timer);
    return res.ok;
  } catch {
    clearTimeout(timer);
    return false;
  }
}

/**
 * Global wrapper around fetch for server API communication.
 * Automatically triggers server-down navigation if the server is unreachable or faults.
 */
export async function apiFetch(endpoint: string, options: RequestInit = {}): Promise<Response> {
  const url = endpoint.startsWith('http') ? endpoint : `${API_BASE_URL}${endpoint}`;
  try {
    const response = await fetch(url, options);
    if (response.status >= 500) {
      triggerServerDown();
      throw new ServerDownError(`Server error (HTTP ${response.status})`);
    }
    return response;
  } catch (err) {
    triggerServerDown();
    if (err instanceof ServerDownError) {
      throw err;
    }
    throw new ServerDownError(err instanceof Error ? err.message : 'Network error');
  }
}
