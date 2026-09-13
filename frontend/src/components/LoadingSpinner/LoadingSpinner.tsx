import styles from './LoadingSpinner.module.scss';

export default function LoadingSpinner() {
  return <div className={styles.spinner} role="status" aria-label="Loading" />;
}
