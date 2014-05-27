with Ada.Text_IO, Ada.Integer_Text_IO;
use  Ada.Text_IO, Ada.Integer_Text_IO;

procedure liftovi is

	type pb_t is array (1 .. 4) of Boolean;

	type lift_t is record
		kat	: Integer;
		smjer	: Integer;
		vrata	: Boolean;
		tipke	: pb_t;
	end record;

	type pozivi_t is record
		gore	: Boolean;
		dole	: Boolean;
	end record;

	type tipke_t is array (1 .. 4) of pozivi_t;

	procedure Ispis ( lift1, lift2 : in lift_t; tipke : in tipke_t ) is
		BR_LN : constant := 5;
		BR_ST : constant := 18;
	
		type Prikaz is array (1 .. BR_LN) of String (1 .. BR_ST);
		Slika : Prikaz := (
		--012345678901234567
			"LIFT1: 1  2  3  4 ",
			"LIFT1:            ",
			"LIFT2: 1  2  3  4 ",
			"LIFT2:            ",
			"TIPKE: -  -  -  - ");
	
		--indeksi znaka pojedinog kata
		ind : array (1 .. 4) of Integer := (8, 11, 14, 17) ;
		--lijeva i desna "zagrada" oko broja lifta
		lz1 : Character := '[';
		dz1 : Character := ']';
		lz2 : Character := '[';
		dz2 : Character := ']';

	begin
		if lift1.smjer = 0 then
			if lift1.vrata then
				lz1 := '['; dz1 := ']';
			else
				lz1 := '('; dz1 := ')';
			end if;
		elsif lift1.smjer = 1 then
			lz1 := '('; dz1 := '>';
		elsif lift1.smjer = -1 then
			lz1 := '<'; dz1 := ')';
		end if;

		if lift2.smjer = 0 then
			if lift2.vrata then
				lz2 := '['; dz2 := ']';
			else
				lz2 := '('; dz2 := ')';
			end if;
		elsif lift2.smjer = 1 then
			lz2 := '('; dz2 := '>';
		elsif lift2.smjer = -1 then
			lz2 := '<'; dz2 := ')';
		end if;

		Slika (1) ( ind ( lift1.kat) - 1 ) := lz1;
		Slika (1) ( ind ( lift1.kat) + 1 ) := dz1;
		Slika (3) ( ind ( lift2.kat) - 1 ) := lz2;
		Slika (3) ( ind ( lift2.kat) + 1 ) := dz2;

		for i in 1..4 loop
			if lift1.tipke(i) then
				Slika (2) ( ind (i) ) := 'x';
			end if;
			
			if lift2.tipke(i) then
				Slika (4) ( ind (i) ) := 'x';
			end if;
			
			if tipke(i).gore and tipke(i).dole then
				Slika (5) ( ind (i) ) := 'X';
			elsif tipke(i).gore then
				Slika (5) ( ind (i) ) := 'A';
			elsif tipke(i).dole then
				Slika (5) ( ind (i) ) := 'V';
			else
				Slika (5) ( ind (i) ) := '-';
			end if;
		end loop;

		New_Line (1);
		for i in 1..BR_LN loop
			Put_Line(Slika(i));
		end loop;
		New_Line (1);
	end Ispis;

	lift1, lift2 : lift_t;
	tipke : tipke_t;

begin
	--jedno stanje
	lift1.kat := 2;
	lift1.smjer := 1;
	lift1.vrata := False;
	lift1.tipke(1) := True;
	lift1.tipke(2) := False;
	lift1.tipke(3) := True;
	lift1.tipke(4) := False;

	lift2.kat := 1;
	lift2.smjer := 0;
	lift2.vrata := True;
	lift2.tipke(1) := False;
	lift2.tipke(2) := False;
	lift2.tipke(3) := True;
	lift2.tipke(4) := False;

	tipke(1).gore := False;
	tipke(1).dole := False;
	tipke(2).gore := True;
	tipke(2).dole := True;
	tipke(3).gore := True;
	tipke(3).dole := False;
	tipke(4).gore := False;
	tipke(4).dole := True;

	Ispis ( lift1, lift2, tipke );

	--drugo stanje
	lift1.kat := 3;
	lift1.smjer := 0;
	lift1.vrata := False;
	lift1.tipke(1) := True;
	lift1.tipke(2) := False;
	lift1.tipke(3) := True;
	lift1.tipke(4) := False;

	lift2.kat := 1;
	lift2.smjer := 0;
	lift2.vrata := False;
	lift2.tipke(1) := False;
	lift2.tipke(2) := False;
	lift2.tipke(3) := True;
	lift2.tipke(4) := False;

	Ispis ( lift1, lift2, tipke );

end liftovi;
