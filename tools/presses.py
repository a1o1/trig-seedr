#!/usr/bin/env python3
"""Panel tokens -> dtboot --press arguments (after sidecar-mods/tools/emu/pj_emu.py).
    presses.py START_M TOKEN...      prints the arguments, then '# end <clock>M' on stderr
Tokens: KEY, KEY*N (N presses 50 M apart), HOLD+KEY (HOLD held 80 M, KEY 20 M in; F = FUNC), w:M (wait M)."""
import sys
def presses(tokens, t):
    p = []
    for tok in tokens:
        if tok.startswith("w:"):
            t += int(tok[2:]); continue
        n = 1
        if "*" in tok:
            tok, n = tok.split("*"); n = int(n)
        for _ in range(n):
            if "+" in tok:
                hold, key = tok.split("+", 1)
                p += ["--press", f"{'FUNC' if hold == 'F' else hold}@{t}M:80M", "--press", f"{key}@{t + 20}M"]; t += 130
            else:
                p += ["--press", f"{tok}@{t}M"]; t += 50
    return p, t
if __name__ == "__main__":
    p, t = presses(sys.argv[2:], int(sys.argv[1]))
    print(" ".join(p)); print(f"# end {t}M", file=sys.stderr)
