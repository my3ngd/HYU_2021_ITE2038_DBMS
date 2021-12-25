-- Print the number of Pokémon whose type is not fire

SELECT  COUNT(*)
FROM    Pokemon AS P
WHERE   P.type <> 'fire';
