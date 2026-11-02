# Phase 6 - Cyclomatic Complexity Review

Date: 2 November 2026
Source commit: c35ab7a
Tool: Lizard 1.24.3
Scope: src and include

## Results

Functions analysed: 311
Total Lizard NLOC: 6262
Average NLOC per function: 14.9
Average cyclomatic complexity: 3.6
Maximum cyclomatic complexity: 25
Functions exceeding CCN 15: 5
Additional function length flags: 1

## Functions Selected for Review

| Function | CCN | Review result |
|---|---:|---|
| saveVersion | 25 | Enrollment, deadline and transaction checks inspected |
| saveUnreleasedMark | 23 | Version protection and atomic audit writes inspected |
| validateUtcDateTime | 23 | Format, ranges and leap-year checks inspected |
| BackupManager::verify | 22 | Manifest, checksum and integrity checks inspected |
| approveMark | 18 | Moderation guards and transactional audit inspected |
| AssignmentPanel constructor | 1 | Length warning; mainly GUI initialization |

## Assessment

The average complexity is relatively low, although five
functions contain more complex decision logic.

The flagged functions are associated with important
assessment, submission, backup and validation operations.

Existing automated tests cover relevant successful and
exceptional paths. Complexity warnings do not establish
functional defects.

No refactoring has been performed as a result of this
preliminary review.

## Recommendations

1. Retain regression tests for all flagged business operations.
2. Consider extracting smaller validation and transaction
   helpers during future maintenance.
3. Separate GUI setup into smaller methods if the assignment
   screen grows further.
4. Repeat complexity and regression analysis after refactoring.

## Limitations

Complexity measurements are static indicators only.
The findings require human review and do not prove
correctness or complete test coverage.

Final student self-review and supervisor acceptance
remain separate from this preliminary assessment.
