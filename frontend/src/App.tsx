import { useEffect } from 'react';
import { Routes, Route, useNavigate, useLocation } from 'react-router-dom';
import Login from './pages/Login/Login';
import ErrorPage from './pages/ErrorPage/ErrorPage';
import { pingServer, registerServerDownHandler } from './services/apiClient';

export default function App() {
  const navigate = useNavigate();
  const location = useLocation();

  useEffect(() => {
    // Register global handler for any API fault
    registerServerDownHandler(() => {
      navigate('/500', { replace: true });
    });

    // Skip ping if already on an error route
    if (location.pathname === '/500' || location.pathname === '/403' || location.pathname === '/404' || location.pathname === '/422') {
      return;
    }

    // Startup ping to verify server connectivity
    pingServer().then((isOnline) => {
      if (!isOnline) {
        navigate('/500', { replace: true });
      }
    });
  }, [navigate, location.pathname]);

  return (
    <Routes>
      <Route path="/500" element={<ErrorPage kind="server-down" />} />
      <Route path="/403" element={<ErrorPage kind="not-authorized" />} />
      <Route path="/422" element={<ErrorPage kind="file-corrupted" />} />
      <Route path="/404" element={<ErrorPage kind="not-found" />} />
      <Route path="*" element={<Login />} />
    </Routes>
  );
}
