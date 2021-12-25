-- Print the name of the gym leader in Brown city.

SELECT  T.name
FROM    Gym AS G, Trainer AS T
WHERE   G.city = 'Brown City'
    AND T.id = G.leader_id;
