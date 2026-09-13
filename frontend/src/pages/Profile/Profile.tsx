import { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { User } from '../../models/User';
import { getCurrentUser } from '../../services/authService';
import styles from './Profile.module.scss';

export default function Profile() {
  const [user, setUser] = useState<User | null>(null);

  useEffect(() => {
    setUser(getCurrentUser());
  }, []);

  if (!user) {
    return (
      <p>
        Not logged in. <Link to="/login">Go to Login</Link>
      </p>
    );
  }

  return (
    <div className={styles.profile}>
      <h1>{user.name}</h1>
      <p>{user.email}</p>
    </div>
  );
}
