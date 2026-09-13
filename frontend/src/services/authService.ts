import { User } from '../models/User';

let currentUser: User | null = null;

export function login(email: string, _password: string): Promise<User> {
  currentUser = { id: 'u1', name: 'Guest User', email };
  return Promise.resolve(currentUser);
}

export function logout(): void {
  currentUser = null;
}

export function getCurrentUser(): User | null {
  return currentUser;
}
