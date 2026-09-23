import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import styles from './Welcome.module.scss';

interface User {
  id: number;
  name?: string;
  email: string;
}

export default function Welcome() {
  const [user, setUser] = useState<User | null>(null);
  const navigate = useNavigate();

  useEffect(() => {
    const stored = localStorage.getItem('zemo_user');
    if (!stored) {
      navigate('/login', { replace: true });
      return;
    }
    try {
      setUser(JSON.parse(stored));
    } catch {
      localStorage.removeItem('zemo_user');
      navigate('/login', { replace: true });
    }
  }, [navigate]);

  function handleSignOut() {
    localStorage.removeItem('zemo_user');
    navigate('/login', { replace: true });
  }

  if (!user) {
    return null;
  }

  const displayName = user.name && user.name.trim().length > 0 ? user.name : user.email.split('@')[0];

  return (
    <div className={styles.container}>
      <div className={styles.welcomeCard}>
        <div className={styles.avatarCircle}>
          {displayName.charAt(0).toUpperCase()}
        </div>

        <div className={styles.header}>
          <span className={styles.badge}>Active Session</span>
          <h1>Welcome, {displayName}!</h1>
          <p className={styles.subtitle}>
            You have successfully connected to <strong>ZemoLibrary</strong>.
          </p>
        </div>

        <div className={styles.detailsBox}>
          <div className={styles.detailRow}>
            <span className={styles.detailLabel}>Account Name</span>
            <span className={styles.detailValue}>{user.name || 'Not provided'}</span>
          </div>
          <div className={styles.detailRow}>
            <span className={styles.detailLabel}>Email Address</span>
            <span className={styles.detailValue}>{user.email}</span>
          </div>
          <div className={styles.detailRow}>
            <span className={styles.detailLabel}>User ID</span>
            <span className={styles.detailValue}>#{user.id}</span>
          </div>
          <div className={styles.detailRow}>
            <span className={styles.detailLabel}>Server Status</span>
            <span className={styles.onlineBadge}>● Online (app.zemoserver.dev)</span>
          </div>
        </div>

        <div className={styles.actions}>
          <button type="button" className={styles.signOutBtn} onClick={handleSignOut}>
            Sign Out
          </button>
        </div>
      </div>
    </div>
  );
}
