// Central API Configuration
export const API_BASE_URL: string =
  (import.meta as { env?: { VITE_API_BASE_URL?: string } }).env?.VITE_API_BASE_URL ||
  'https://app.zemoserver.dev';
