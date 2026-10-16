import type { Color, Score } from '../api.ts';
import { formatScore, whiteShare } from '../chess-utils.ts';

export function EvalBar({ score, orientation }: { score: Score | null; orientation: Color }) {
  const white = whiteShare(score);
  const text = formatScore(score);
  return (
    <div
      className={`eval-bar ${orientation === 'black' ? 'flipped' : ''}`}
      role="meter"
      aria-label="Engine evaluation for White"
      aria-valuemin={0}
      aria-valuemax={100}
      aria-valuenow={Math.round(white)}
      aria-valuetext={text}
      title={`Evaluation (White's view): ${text}`}
    >
      <div className="eval-white" style={{ height: `${white}%` }} />
      <span className={`eval-label ${white >= 50 ? 'on-white' : 'on-black'}`}>{text}</span>
    </div>
  );
}
