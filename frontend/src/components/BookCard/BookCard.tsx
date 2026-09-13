import { Link } from 'react-router-dom';
import { Book } from '../../models/Book';
import RatingStars from '../RatingStars/RatingStars';
import styles from './BookCard.module.scss';

interface BookCardProps {
  book: Book;
}

export default function BookCard({ book }: BookCardProps) {
  return (
    <Link to={`/catalog/${book.id}`} className={styles.card}>
      <img src={book.coverUrl} alt={book.title} className={styles.cover} />
      <h3 className={styles.title}>{book.title}</h3>
      <p className={styles.author}>{book.author}</p>
      <RatingStars rating={book.rating} />
    </Link>
  );
}
