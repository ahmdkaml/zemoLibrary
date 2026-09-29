import { Routes, Route } from 'react-router-dom';
import Login from './pages/Login/Login';
import ErrorPage from './pages/ErrorPage/ErrorPage';
import Home from './pages/Home/Home';
import Layout from './components/Layout/Layout';

export default function App() {
  return (
    <Routes>
      <Route element={<Layout />}>
        <Route path="*" element={<Home />} />
              <Route path="/login" element={<Login />} />
              <Route path="/403" element={<ErrorPage kind="not-authorized" />} />
              <Route path="/500" element={<ErrorPage kind="server-down" />} />
              <Route path="/422" element={<ErrorPage kind="file-corrupted" />} />
              <Route path="/404" element={<ErrorPage kind="not-found" />} />
      </Route>
    </Routes>
  );
}
