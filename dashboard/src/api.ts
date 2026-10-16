export type Color = 'white' | 'black';

export interface MoveInfo {
  uci: string;
  san: string;
}

/** White's point of view: centipawns, or moves to mate (negative when White is mated). */
export interface Score {
  cp: number | null;
  mate: number | null;
}

export interface GameState {
  id: number;
  created: number;
  fen: string;
  start_fen: string;
  player_color: Color;
  level: number;
  turn: Color;
  moves: MoveInfo[];
  status: string;
  result: string | null;
  in_check: boolean;
  legal_moves: string[];
  evaluation: Score | null;
}

export interface Analysis {
  fen: string;
  status: string;
  score: Score | null;
  best_move: MoveInfo | null;
  pv: MoveInfo[];
  depth: number;
  nodes: number;
}

export interface EngineInfo {
  name: string;
  evaluation: 'classical' | 'nnue';
  eval_file: string | null;
  levels: Record<string, string>;
}

export class ApiError extends Error {
  readonly status: number;

  constructor(status: number, message: string) {
    super(message);
    this.status = status;
  }
}

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`/api${path}`, {
    ...init,
    headers: { 'Content-Type': 'application/json', ...init?.headers },
  });
  if (!response.ok) {
    let message = response.statusText;
    try {
      const { detail } = (await response.json()) as { detail?: unknown };
      if (typeof detail === 'string') message = detail;
      else if (Array.isArray(detail)) message = detail.map((d: { msg?: string }) => d.msg).join('; ');
    } catch {
      // Keep the status text when the body is not JSON
    }
    throw new ApiError(response.status, message);
  }
  return (await response.json()) as T;
}

const post = <T>(path: string, body?: unknown) =>
  request<T>(path, { method: 'POST', body: body === undefined ? undefined : JSON.stringify(body) });

export const api = {
  health: () => request<{ status: string; engine: string }>('/health'),
  engine: () => request<EngineInfo>('/engine'),
  listGames: () => request<GameState[]>('/games'),
  getGame: (id: number) => request<GameState>(`/games/${id}`),
  createGame: (body: { player_color: Color; level: number; fen?: string }) => post<GameState>('/games', body),
  move: (id: number, move: string) => post<GameState>(`/games/${id}/moves`, { move }),
  undo: (id: number) => post<GameState>(`/games/${id}/undo`),
  resign: (id: number) => post<GameState>(`/games/${id}/resign`),
  analyze: (fen: string, movetimeMs: number) => post<Analysis>('/analyze', { fen, movetime_ms: movetimeMs }),
  pgnUrl: (id: number) => `/api/games/${id}/pgn`,
};
