import { Link } from 'react-router-dom';
import styles from './Header.module.scss';

export default function Header() {
  return (
    <header className={styles.header}>
      <Link to="/" className={styles.logo}>Zemolibrary</Link>
    </header>
  );
}
