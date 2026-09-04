# SAE / II-42 Research Report Archive

Updated: 2026-09-03

This directory is the canonical archive for research plans, experiment reports,
design notes, milestone summaries, and route-closure documents. Research
Markdown files no longer live in the repository root.

Files whose names contain `plan` or `roadmap` are historical experiment
records. They are not active engineering TODO authorities. Current storage and
lifecycle behavior is defined by the
[Convergent Segmented Index](../../convergent-segmented-index.md).

The bilingual model technical reports have been promoted out of this archive:
[English](../../technical-report-ii42-model.md) and
[Traditional Chinese](../../technical-report-ii42-model-zh.md). Historical evaluation
reports stay below; detailed active follow-up is in
[Model Planning](../../model-planning.md), subordinate to the product roadmap.

The archive is organized by experiment number so that each directory remains
small enough to browse on GitHub. Files without an II-42 experiment number use
the `designs/` or `milestones/` collections. The older pre-numbered SAE archive
remains directly in this directory to preserve its established links.

Machine-readable evidence is not part of the product source tree. The frozen
archive, checksums, and path mapping are documented in
[Research Artifacts](../artifacts/README.md). Small fixtures required
by tests remain under `tests/fixtures/research/`; active experiment output
belongs under the ignored `runs/` directory.

## Numbered Stages

| Stage | Documents |
| --- | ---: |
| [M0100-M0199](m0100-m0199/) | 51 |
| [M0200-M0299](m0200-m0299/) | 2 |
| [M0300-M0399](m0300-m0399/) | 76 |
| [M0400-M0499](m0400-m0499/) | 30 |
| [M0500-M0599](m0500-m0599/) | 77 |
| [M0600-M0699](m0600-m0699/) | 236 |
| [M0700-M0799](m0700-m0799/) | 137 |
| [M0800-M0899](m0800-m0899/) | 50 |
| [M1000-M1099](m1000-m1099/) | 8 |
| [M1100-M1199](m1100-m1199/) | 191 |
| [M1200-M1299](m1200-m1299/) | 100 |
| [M1300-M1399](m1300-m1399/) | 36 |
| [M1400-M1499](m1400-m1499/) | 3 |
| [M1500-M1599](m1500-m1599/) | 74 |
| [M1600-M1699](m1600-m1699/) | 37 |
| [M1700-M1799](m1700-m1799/) | 30 |
| [M1800-M1899](m1800-m1899/) | 14 |
| [M1900-M1999](m1900-m1999/) | 93 |

Numbers absent from this table currently have no root-level research Markdown
documents to archive. New reports should be written directly to the matching
stage directory rather than added to the repository root.

## Curated Collections

- [Design documents](designs/)
- [Historical hybrid Vector/BM25 deployment design](designs/ii42-hybrid-vector-bm25-use-case-design.md)
- [Historical online-maintenance design](designs/ii42-online-maintenance-future-plan.md)
- [Milestone and current-state summaries](milestones/)
- [M1900-M1951 learned-sparse milestone](m1900-m1999/ii42-m1900-m1951-learned-sparse-one-index-milestone.md)
- [M1934 unseen-transfer report](m1900-m1999/ii42-m1934-fixed-budget-unseen-transfer-report.md)

## Legacy Archive

The early SAE archive remains directly in this directory. It uses pre-M100,
phase, milestone, or descriptive naming and covers the original SAE, SPLADE,
block-max, candidate-budget, PostgreSQL, and unified-payload work. Keeping
those stable avoids rewriting a large historical link graph while numbered
M100-and-later reports stay in their matching stage directories.

Historical bare Markdown filenames should be interpreted relative to this
archive. Bare JSON, JSONL, checksum, checkpoint, or log filenames should be
looked up by `original_path` in the
[research artifact manifest](../artifacts/manifest.jsonl). For
numbered II-42 documents, use the experiment number to select the matching
report stage directory.

## Archive Rules

- Do not delete experiment reports when a route closes; move them here.
- Keep reports, plans, and conclusions as Markdown.
- Keep raw evidence under ignored `runs/` while active, then freeze it in the
  external artifact archive.
- Write new numbered reports directly into their stage directory.
- Update links and runner output defaults when moving a document.
- Create a new stage directory only when the first report in that range exists.
