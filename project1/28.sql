-- Print the names of trainers from Brown city or Rainbow city in alphabetical order

SELECT  T.name
FROM    Trainer AS T
WHERE   T.hometown LIKE 'Brown City'
    OR  T.hometown LIKE 'Rainbow City'
ORDER BY T.name ASC;
