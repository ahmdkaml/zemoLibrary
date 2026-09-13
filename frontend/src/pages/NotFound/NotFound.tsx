import { Link } from 'react-router-dom';
import styles from './NotFound.module.scss';

export default function NotFound() {
  return (
    <div className={styles.notFound}>
      <h1>404</h1>
      <p>Page not found.</p>
      <Link to="/">Back home</Link>
    </div>
  );
}
