import { NavLink } from 'react-router-dom';
import styles from './Navbar.module.scss';

const LINKS = [
  { to: '/', label: 'Home' },
  { to: '/catalog', label: 'Catalog' },
  { to: '/search', label: 'Search' },
  { to: '/login', label: 'Login' },
  { to: '/profile', label: 'Profile' }
];

export default function Navbar() {
  return (
    <nav className={styles.nav}>
      {LINKS.map((link) => (
        <NavLink
          key={link.to}
          to={link.to}
          className={({ isActive }) => (isActive ? styles.active : styles.link)}
        >
          {link.label}
        </NavLink>
      ))}
    </nav>
  );
}
