import { Link } from 'react-router-dom';
import styles from './ErrorPage.module.scss';

type ErrorKind = 'not-found' | 'not-authorized' | 'server-down' | 'file-corrupted';

const CONTENT: Record<ErrorKind, { code: string; title: string; message: string }> = {
  'not-found': {
    code: '404',
    title: 'Not Found',
    message: 'This page does not exist.'
  },
  'not-authorized': {
    code: '403',
    title: 'Not Authorized',
    message: 'You lack access to this page.'
  },
  'server-down': {
    code: '500',
    title: 'Server Down',
    message: 'Server unreachable. Try again later.'
  },
  'file-corrupted': {
    code: '422',
    title: 'File Corrupted',
    message: 'This file cannot be read.'
  }
};

interface ErrorPageProps {
  kind: ErrorKind;
}

export default function ErrorPage({ kind }: ErrorPageProps) {
  const { code, title, message } = CONTENT[kind];

  return (
    <div className={styles.error}>
      <h1 className={styles.code}>{code}</h1>
      <h2 className={styles.title}>{title}</h2>
      <p className={styles.message}>{message}</p>
      <Link to="/" className={styles.link}>Back home</Link>
    </div>
  );
}
