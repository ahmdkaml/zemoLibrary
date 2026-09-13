import { Book } from '../models/Book';
import { getAll } from './bookService';

export async function searchBooks(query: string): Promise<Book[]> {
  const books = await getAll();
  const q = query.trim().toLowerCase();
  if (!q) return books;
  return books.filter(
    (b) =>
      b.title.toLowerCase().includes(q) ||
      b.author.toLowerCase().includes(q) ||
      b.genre.toLowerCase().includes(q)
  );
}
