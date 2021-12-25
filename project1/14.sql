-- Print the names of the cities with the most native trainers.

SELECT  Q.name
FROM    (
    SELECT  Ci.name, COUNT(*) AS cnt
    FROM    City AS Ci, Trainer AS T
    WHERE   Ci.name = T.hometown
    GROUP BY Ci.name
    ) AS Q
WHERE   Q.cnt >= ALL(
    SELECT  q.cnt
    FROM    (
        SELECT  Ci.name, COUNT(*) AS cnt
        FROM    City AS Ci, Trainer AS T
        WHERE   Ci.name = T.hometown
        GROUP BY Ci.name
        ) AS q
    ) 
ORDER BY Q.name ASC;
