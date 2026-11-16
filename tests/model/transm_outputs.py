"""Read TRANSM's tab-separated trace tables (the *.xls outputs).

Each table has one or more group header rows, then a row of column names that
starts with "Month" or "Time", then one row per month. A group label covers the
columns to its right until the next label, so a column is named by joining the
labels above it with " / ", e.g. "Deaths / Total" or
"Prevalent Cases / By Gender / MALE".
"""

import csv
import math


class Table:
    def __init__(self, path):
        with open(path, newline="") as f:
            rows = list(csv.reader(f, delimiter="\t"))
        header_at = next((i for i, r in enumerate(rows) if r and r[0] in ("Month", "Time")), None)
        if header_at is None:
            raise ValueError(f"{path}: no 'Month' or 'Time' header row")

        width = max(len(r) for r in rows[: header_at + 1])
        groups = []
        for r in rows[:header_at]:
            filled, label = [], ""
            for i in range(width):
                cell = r[i].strip() if i < len(r) else ""
                if cell:
                    label = cell
                filled.append(label)
            groups.append(filled)

        self.path = path
        self.columns = []
        for i, name in enumerate(rows[header_at] + [""] * (width - len(rows[header_at]))):
            parts = [g[i] for g in groups if g[i]] + ([name.strip()] if name.strip() else [])
            self.columns.append(" / ".join(parts))

        self.months = []
        self.rows = []
        for r in rows[header_at + 1:]:
            if not r or not r[0].strip():
                continue
            month = r[0].strip()
            # Monthly rows are "init" then 1, 2, ...; summary rows such as
            # "Population Avg. LM" follow them and are not read.
            if month.lower() != "init" and not month.isdigit():
                break
            self.months.append(month)
            self.rows.append([_number(c) for c in r[1:]] + [math.nan] * (width - len(r)))

    def find(self, *parts):
        """Index of the one column whose name contains every part, matched as whole labels."""
        hits = [i for i, c in enumerate(self.columns)
                if all(p in c.split(" / ") for p in parts)]
        if len(hits) != 1:
            raise KeyError(f"{self.path}: {len(hits)} columns match {parts}: "
                           + "; ".join(self.columns[i] for i in hits[:5]))
        return hits[0]

    def column(self, *parts):
        """Values of one column, one per data row (the 'init' row included)."""
        i = self.find(*parts) - 1
        return [row[i] for row in self.rows]


def _number(cell):
    cell = cell.strip()
    if not cell:
        return math.nan
    try:
        return float(cell)
    except ValueError:
        return math.nan
