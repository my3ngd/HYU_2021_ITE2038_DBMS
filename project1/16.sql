-- Print the name of the city's gym leader with Amazon characteristics.

SELECT  T.name
FROM    Trainer AS T, Gym AS G, City AS C
WHERE   T.id = G.leader_id
    AND G.city = C.name
    AND C.description = 'Amazon';
