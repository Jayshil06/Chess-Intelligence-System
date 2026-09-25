"""Elo estimation and SPRT (logistic Elo, trinomial normal approximation) for engine matches."""
from __future__ import annotations

import math
from dataclasses import dataclass


def expected_score(elo: float) -> float:
    return 1.0 / (1.0 + 10.0 ** (-elo / 400.0))


def elo_from_score(score: float) -> float:
    if score <= 0.0:
        return -math.inf
    if score >= 1.0:
        return math.inf
    return -400.0 * math.log10(1.0 / score - 1.0)


def _score_and_variance(wins: int, draws: int, losses: int) -> tuple[int, float, float]:
    n = wins + draws + losses
    if n == 0:
        return 0, 0.5, 0.0
    score = (wins + 0.5 * draws) / n
    variance = (wins * (1 - score) ** 2 + draws * (0.5 - score) ** 2 + losses * score ** 2) / n
    return n, score, variance


@dataclass(frozen=True)
class EloEstimate:
    elo: float
    low: float   # 95% confidence interval
    high: float

    def __str__(self) -> str:
        return f"{self.elo:+.1f} [{self.low:+.1f}, {self.high:+.1f}]"


def estimate_elo(wins: int, draws: int, losses: int) -> EloEstimate:
    n, score, variance = _score_and_variance(wins, draws, losses)
    if n == 0:
        raise ValueError("no games played")
    margin = 1.96 * math.sqrt(variance / n)
    return EloEstimate(elo_from_score(score), elo_from_score(score - margin), elo_from_score(score + margin))


def sprt_bounds(alpha: float = 0.05, beta: float = 0.05) -> tuple[float, float]:
    return math.log(beta / (1 - alpha)), math.log((1 - beta) / alpha)


def sprt_llr(wins: int, draws: int, losses: int, elo0: float, elo1: float) -> float:
    """Log-likelihood ratio of H1 (Elo = elo1) against H0 (Elo = elo0)."""
    n, score, variance = _score_and_variance(wins, draws, losses)
    if n == 0 or variance == 0.0:
        return 0.0
    s0, s1 = expected_score(elo0), expected_score(elo1)
    return n * (s1 - s0) * (2 * score - s0 - s1) / (2 * variance)


def sprt_decision(wins: int, draws: int, losses: int, elo0: float = 0.0, elo1: float = 5.0,
                  alpha: float = 0.05, beta: float = 0.05) -> str | None:
    """"H1" (change is stronger), "H0" (it is not), or None to keep playing."""
    llr = sprt_llr(wins, draws, losses, elo0, elo1)
    lower, upper = sprt_bounds(alpha, beta)
    if llr >= upper:
        return "H1"
    if llr <= lower:
        return "H0"
    return None
