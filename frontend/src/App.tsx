import { Routes, Route } from 'react-router-dom';
import Login from './pages/Login/Login';
import ErrorPage from './pages/ErrorPage/ErrorPage';

export default function App() {
  return (
    <Routes>
              <Route path="*" element={<Login />} />
              <Route path="/403" element={<ErrorPage kind="not-authorized" />} />
              <Route path="/500" element={<ErrorPage kind="server-down" />} />
              <Route path="/422" element={<ErrorPage kind="file-corrupted" />} />
              <Route path="/404" element={<ErrorPage kind="not-found" />} />

    </Routes>
  );
}
