# Log Manager & Crash Recovery ♻

DBMS experiences crash in many situations. As a result, if the ongoing transaction is forcibly terminated, Atomicity and Durability will not be guaranteed. To compensate for this situation, DBMS use Log Manager. When crash occurs, Redo and Undo operations are performed using the log left when restarting DBMS. Among them, DBMSs use the ARIES recovery algorithm.

## Logs

In the recovery process, transactions are divided into winner and loser. Winner is a successful transaction, and loser is a failed transaction. One of the algorithms used to reverse the loser reflected on the disk and the unreflected winner is the ARIES algorithm.

There are 5 types of logs:

1. Begin: Log indicating that transaction has started.
2. Update: Log indicating that transaction has updated record.
3. Commit: Log indicating that transaction has successfully ended.
4. Rollback: Log indicating that transaction has failed, rollback operation runned.
5. Compensate: Log indicating that the Undo operation will proceed.

The created log is first stored in the log buffer. If all logs are left directly in the log file, it will have a very bad impact on the performance of the DBMS, so it will create and manage buffers on memory. Log buffer flushes according to a certain situation, which is the following situation.

1. `trx_commit` called
2. `trx_abort` called
3. eviction occured in buffer manager

When the DBMS stops, the log buffer on the memory also disappears, so it is recommended to flush the log frequently. (However, the more flushes, the less the normal DBMS performance.)

## 3 Pass Recovery Algorithm

In the 3 Pass recovery algorithm, DBMSs will go through 3 passes.

1. Analysis pass: Read the log and classify the winner transaction and loser transaction.

2. Redo pass: Tour log forward and redo log.

    At this time, compare the log sequence number (LSN) and the page LSN and perform the consider-redo if `page_LSN >= log_LSN`.

3. Undo pass: Tour log backward and undo log.

    At this time, the composite log means that the original operation undo was performed, so it does not undo again. (Compensate log reduces the undo operation, resulting in a monotone decrease recovery time even in repetitive crashes.)

---

> ※전공 과목 6개의 시험기간과 겹쳐 log manager를 거의 구현을 못했습니다... 한 학기동안 교수님과 조교님들 모두 수고 많으셨습니다. 감사합니다 🙇
