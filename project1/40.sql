-- Print the names of the trainers who caught Pikachu in alphabetical order.

SELECT  T.name
FROM    Trainer AS T, CatchedPokemon AS CP, Pokemon AS P
WHERE   P.name = 'Pikachu'
    AND P.id = CP.pid
    AND CP.owner_id = T.id
ORDER BY T.name;
