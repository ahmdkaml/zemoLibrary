import { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { Book } from '../../models/Book';
import { getById } from '../../services/bookService';
import { addToCart } from '../../services/cartService';
import RatingStars from '../../components/RatingStars/RatingStars';
import LoadingSpinner from '../../components/LoadingSpinner/LoadingSpinner';
import styles from './BookDetail.module.scss';

export default function BookDetail() {
  const { id } = useParams<{ id: string }>();
  const [book, setBook] = useState<Book | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    if (!id) return;
    getById(id).then((data) => {
      setBook(data ?? null);
      setLoading(false);
    });
  }, [id]);

  if (loading) return <LoadingSpinner />;
  if (!book) return <p>Book not found. <Link to="/catalog">Back to catalog</Link></p>;

  return (
    <div className={styles.detail}>
      <img src={book.coverUrl} alt={book.title} className={styles.cover} />
      <div>
        <h1>{book.title}</h1>
        <p className={styles.author}>{book.author}</p>
        <RatingStars rating={book.rating} />
        <p className={styles.description}>{book.description}</p>
        <button onClick={() => addToCart(book)}>Add to Cart</button>
      </div>
    </div>
  );
}
