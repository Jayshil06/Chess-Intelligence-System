import { Crown } from 'lucide-react';
import { NavLink, Outlet } from 'react-router';

const LINKS = [
  { to: '/', label: 'Home', end: true },
  { to: '/play', label: 'Play' },
  { to: '/analysis', label: 'Analysis' },
  { to: '/games', label: 'Games' },
  { to: '/engine', label: 'Engine' },
];

export function Layout() {
  return (
    <>
      <header className="site-header">
        <NavLink to="/" className="brand" aria-label="Chess Intelligence home">
          <Crown size={22} aria-hidden />
          <span>Chess Intelligence</span>
        </NavLink>
        <nav aria-label="Main">
          {LINKS.map((link) => (
            <NavLink key={link.to} to={link.to} end={link.end}>
              {link.label}
            </NavLink>
          ))}
        </nav>
      </header>
      <main>
        <Outlet />
      </main>
    </>
  );
}
