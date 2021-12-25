-- Print the names of trainers from Blue city in alphabetical order.

SELECT DISTINCT T.name
FROM    Trainer AS T, City AS C
WHERE   T.hometown = 'Blue City'
ORDER BY T.name ASC;
