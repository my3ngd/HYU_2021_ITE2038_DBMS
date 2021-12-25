-- Among the catched Pokémon, print the nicknames of the Pokémon
-- with a level 40 or higher and an owner_id of 5 or higher in alphabetical order

SELECT  CP.nickname
FROM    CatchedPokemon AS CP
WHERE   CP.level >= 40
    AND CP.owner_id >= 5
ORDER BY CP.nickname;
