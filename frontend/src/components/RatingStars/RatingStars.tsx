import styles from './RatingStars.module.scss';

interface RatingStarsProps {
  rating: number;
  max?: number;
}

export default function RatingStars({ rating, max = 5 }: RatingStarsProps) {
  const stars = Array.from({ length: max }, (_, i) => i < Math.round(rating));
  return (
    <div className={styles.stars} aria-label={`Rating: ${rating} out of ${max}`}>
      {stars.map((filled, i) => (
        <span key={i} className={filled ? styles.filled : styles.empty}>
          ★
        </span>
      ))}
    </div>
  );
}
