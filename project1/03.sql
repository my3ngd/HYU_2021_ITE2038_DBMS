-- Print the name of the trainer, not the gym leader, in alphabetical order.

SELECT  T.name
FROM    Trainer AS T
WHERE   T.id NOT IN (
    SELECT  G.leader_id
    FROM    Gym AS G
    )
ORDER BY T.name ASC;
