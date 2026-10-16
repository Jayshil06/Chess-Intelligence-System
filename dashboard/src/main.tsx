import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { BrowserRouter, Route, Routes } from 'react-router';
import { Layout } from './Layout.tsx';
import { Analysis } from './pages/Analysis.tsx';
import { Engine } from './pages/Engine.tsx';
import { Games } from './pages/Games.tsx';
import { Home } from './pages/Home.tsx';
import { NotFound } from './pages/NotFound.tsx';
import { Play } from './pages/Play.tsx';
import './styles.css';

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BrowserRouter>
      <Routes>
        <Route element={<Layout />}>
          <Route index element={<Home />} />
          <Route path="play/:gameId?" element={<Play />} />
          <Route path="analysis" element={<Analysis />} />
          <Route path="games/:gameId?" element={<Games />} />
          <Route path="engine" element={<Engine />} />
          <Route path="*" element={<NotFound />} />
        </Route>
      </Routes>
    </BrowserRouter>
  </StrictMode>,
);
