import { FormEvent, useState } from 'react';
import styles from './Login.module.scss';
import { apiFetch } from '../../services/apiClient';

type Mode = 'signin' | 'signup';

interface LoggedInUser {
  id: number;
  name?: string;
  email: string;
}

export default function Login() {
  const [mode, setMode] = useState<Mode>('signin');
  const [name, setName] = useState('');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [loggedInUser, setLoggedInUser] = useState<LoggedInUser | null>(null);
  const [welcomeMessage, setWelcomeMessage] = useState<string | null>(null);

  function toggleMode() {
    setMode((m) => (m === 'signin' ? 'signup' : 'signin'));
    setError(null);
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setLoading(true);

    try {
      if (mode === 'signup') {
        if (password !== confirmPassword) {
          setError('Passwords do not match.');
          setLoading(false);
          return;
        }
        const res = await apiFetch('/auth/signup', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ name, email, password }),
        });
        const data = await res.json();
        if (!data.success) {
          setError(data.message || 'Signup failed.');
          setLoading(false);
          return;
        }
        setLoggedInUser(data.user);
        setWelcomeMessage(`Welcome to ZemoLibrary, ${data.user?.name || data.user?.email}!`);
      } else {
        const res = await apiFetch('/auth/login', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ email, password }),
        });
        const data = await res.json();
        if (!data.success) {
          setError(data.message || 'Login failed.');
          setLoading(false);
          return;
        }
        setLoggedInUser(data.user);
        setWelcomeMessage(`Welcome back, ${data.user?.name || data.user?.email}!`);
      }
    } catch {
      // apiFetch automatically triggers server-down navigation on network fault
      setError('Unable to reach server.');
    } finally {
      setLoading(false);
    }
  }

  if (loggedInUser && welcomeMessage) {
    return (
      <div className={styles.wrapper}>
        <div className={styles.welcomeCard}>
          <div className={styles.successBadge}>✓</div>
          <h2>{welcomeMessage}</h2>
          <p className={styles.userInfo}>
            Logged in as <strong>{loggedInUser.email}</strong>
          </p>
          <button
            type="button"
            className={styles.signOutButton}
            onClick={() => {
              setLoggedInUser(null);
              setWelcomeMessage(null);
              setPassword('');
              setConfirmPassword('');
            }}
          >
            Sign Out / Switch Account
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className={styles.wrapper}>
      <form className={styles.form} onSubmit={handleSubmit}>
        <h1>{mode === 'signin' ? 'Sign In' : 'Sign Up'}</h1>

        {mode === 'signup' && (
          <input
            type="text"
            placeholder="Full name"
            value={name}
            onChange={(e) => setName(e.target.value)}
            required
          />
        )}

        <input
          type="email"
          placeholder="Email"
          value={email}
          onChange={(e) => setEmail(e.target.value)}
          required
        />

        <input
          type="password"
          placeholder="Password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
          required
        />

        {mode === 'signup' && (
          <input
            type="password"
            placeholder="Confirm password"
            value={confirmPassword}
            onChange={(e) => setConfirmPassword(e.target.value)}
            required
          />
        )}

        {error && <p className={styles.error}>{error}</p>}

        <button type="submit" disabled={loading}>
          {loading ? 'Please wait...' : mode === 'signin' ? 'Sign In' : 'Sign Up'}
        </button>

        <p className={styles.switch}>
          {mode === 'signin' ? "Don't have an account?" : 'Already have an account?'}{' '}
          <button type="button" className={styles.switchLink} onClick={toggleMode}>
            {mode === 'signin' ? 'Sign Up' : 'Sign In'}
          </button>
        </p>
      </form>
    </div>
  );
}
