import { Book } from '../models/Book';

let cart: Book[] = [];

export function addToCart(book: Book): void {
  cart.push(book);
}

export function removeFromCart(bookId: string): void {
  cart = cart.filter((b) => b.id !== bookId);
}

export function getCart(): Book[] {
  return cart;
}

export function clearCart(): void {
  cart = [];
}
