import { useEffect, useState } from 'react';
import { Book } from '../../models/Book';
import { getAll } from '../../services/bookService';
import BookCard from '../../components/BookCard/BookCard';
import Sidebar from '../../components/Sidebar/Sidebar';
import LoadingSpinner from '../../components/LoadingSpinner/LoadingSpinner';
import Pagination from '../../components/Pagination/Pagination';
import styles from './Catalog.module.scss';

const PAGE_SIZE = 6;

export default function Catalog() {
  const [books, setBooks] = useState<Book[]>([]);
  const [loading, setLoading] = useState(true);
  const [genre, setGenre] = useState<string | null>(null);
  const [page, setPage] = useState(1);

  useEffect(() => {
    getAll().then((data) => {
      setBooks(data);
      setLoading(false);
    });
  }, []);

  const filtered = genre ? books.filter((b) => b.genre === genre) : books;
  const totalPages = Math.max(1, Math.ceil(filtered.length / PAGE_SIZE));
  const pageItems = filtered.slice((page - 1) * PAGE_SIZE, page * PAGE_SIZE);

  if (loading) return <LoadingSpinner />;

  return (
    <div className={styles.catalog}>
      <Sidebar
        onSelectGenre={(g) => {
          setGenre(g === genre ? null : g);
          setPage(1);
        }}
      />
      <div className={styles.main}>
        <div className={styles.grid}>
          {pageItems.map((book) => (
            <BookCard key={book.id} book={book} />
          ))}
        </div>
        <Pagination page={page} totalPages={totalPages} onPageChange={setPage} />
      </div>
    </div>
  );
}
