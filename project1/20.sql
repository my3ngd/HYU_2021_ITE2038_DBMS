-- Print the names of the gym leaders in alphabetical order.

SELECT  T.name
FROM    Trainer AS T, Gym AS G
WHERE   T.id = G.leader_id
ORDER BY T.name ASC;
