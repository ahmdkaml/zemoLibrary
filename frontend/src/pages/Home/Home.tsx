import { Link } from 'react-router-dom';
import styles from './Home.module.scss';

export default function Home() {
  return (
    <div className={styles.home}>
      <h1>Welcome to Zemolibrary</h1>
      <p>Your catalog of books, ready to explore.</p>
      <Link to="/catalog" className={styles.cta}>
        Browse Catalog
      </Link>
    </div>
  );
}
