import styles from './Sidebar.module.scss';

const GENRES = ['Mystery', 'Historical Fiction', 'Sci-Fi', 'Romance', 'Non-Fiction'];

interface SidebarProps {
  onSelectGenre?: (genre: string) => void;
}

export default function Sidebar({ onSelectGenre }: SidebarProps) {
  return (
    <aside className={styles.sidebar}>
      <h4>Genres</h4>
      <ul>
        {GENRES.map((genre) => (
          <li key={genre}>
            <button onClick={() => onSelectGenre?.(genre)}>{genre}</button>
          </li>
        ))}
      </ul>
    </aside>
  );
}
