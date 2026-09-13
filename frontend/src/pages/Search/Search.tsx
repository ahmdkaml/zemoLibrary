import { useState } from 'react';
import { Book } from '../../models/Book';
import { searchBooks } from '../../services/searchService';
import BookCard from '../../components/BookCard/BookCard';
import styles from './Search.module.scss';

export default function Search() {
  const [query, setQuery] = useState('');
  const [results, setResults] = useState<Book[]>([]);

  async function handleChange(value: string) {
    setQuery(value);
    setResults(await searchBooks(value));
  }

  return (
    <div className={styles.search}>
        <input aria-label="Search by title, author, or genre"
        type="text"
        placeholder="Search by title, author, or genre"
        value={query}
        onChange={(e) => handleChange(e.target.value)}
        className={styles.input}
      />
      <div className={styles.grid}>
        {results.map((book) => (
          <BookCard key={book.id} book={book} />
        ))}
      </div>
    </div>
  );
}
