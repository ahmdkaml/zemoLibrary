import { Book } from '../models/Book';

const MOCK_BOOKS: Book[] = [
  {
    id: '1',
    title: 'The Silent Library',
    author: 'Amara Osei',
    coverUrl: 'https://placehold.co/200x300',
    description: 'A mystery unfolding inside an ancient library.',
    genre: 'Mystery',
    rating: 4.5,
    publishedYear: 2019
  },
  {
    id: '2',
    title: 'Echoes of Cairo',
    author: 'Youssef Hassan',
    coverUrl: 'https://placehold.co/200x300',
    description: 'A historical drama set in old Cairo.',
    genre: 'Historical Fiction',
    rating: 4.2,
    publishedYear: 2021
  },
  {
    id: '3',
    title: 'Code & Ink',
    author: 'Lina Farouk',
    coverUrl: 'https://placehold.co/200x300',
    description: 'A programmer discovers a hidden manuscript.',
    genre: 'Sci-Fi',
    rating: 4.8,
    publishedYear: 2023
  }
];

export function getAll(): Promise<Book[]> {
  return Promise.resolve(MOCK_BOOKS);
}

export function getById(id: string): Promise<Book | undefined> {
  return Promise.resolve(MOCK_BOOKS.find((b) => b.id === id));
}
