// Central API Configuration
// Currently points to placeholder URL until server configuration is shared
export const API_BASE_URL: string =
  (import.meta as { env?: { VITE_API_BASE_URL?: string } }).env?.VITE_API_BASE_URL ||
  'https://example.com';
